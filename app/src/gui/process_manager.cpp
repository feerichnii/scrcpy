#include "gui/process_manager.h"

#include <cstring>
#include <vector>

extern "C" {
#include "util/file.h"
}

namespace scgui {

ProcessManager::ProcessManager(LogManager *logs)
    : logs_(logs) {
}

ProcessManager::~ProcessManager() {
    std::vector<std::string> ids;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto &kv : processes_) {
            ids.push_back(kv.first);
        }
    }
    for (const auto &id : ids) {
        stop(id);
    }
}

void
ProcessManager::set_scrcpy_executable(const std::string &path) {
    std::lock_guard<std::mutex> lock(mutex_);
    scrcpy_exe_ = path.empty() ? "scrcpy" : path;
}

void
ProcessManager::reader_loop(ManagedProcess *proc, sc_pipe pipe, bool /*is_stderr*/) {
    char buf[512];
    std::string pending;
    while (true) {
        ssize_t r = sc_pipe_read(pipe, buf, sizeof(buf));
        if (r <= 0) {
            break;
        }
        pending.append(buf, static_cast<size_t>(r));
        size_t pos;
        while ((pos = pending.find('\n')) != std::string::npos) {
            std::string line = pending.substr(0, pos);
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (logs_) {
                logs_->append(proc->id, line);
            }
            pending.erase(0, pos + 1);
        }
    }
    if (!pending.empty() && logs_) {
        logs_->append(proc->id, pending);
    }
}

void
ProcessManager::wait_loop(ManagedProcess *proc) {
    sc_exit_code code = sc_process_wait(proc->pid, true);
    proc->exit_code = static_cast<int>(code);
    proc->running = false;

    if (proc->stdout_thread.joinable()) {
        proc->stdout_thread.join();
    }
    if (proc->stderr_thread.joinable()) {
        proc->stderr_thread.join();
    }
    if (proc->pout_valid) {
        sc_pipe_close(proc->pout);
        proc->pout_valid = false;
    }
    if (proc->perr_valid) {
        sc_pipe_close(proc->perr);
        proc->perr_valid = false;
    }

    if (logs_) {
        logs_->append(proc->id, "Process exited with code " + std::to_string(static_cast<int>(code)));
    }
    if (proc->on_exit) {
        proc->on_exit(static_cast<int>(code));
    }
}

ProcessHandle
ProcessManager::start(const std::string &id,
                      const std::vector<std::string> &args,
                      std::function<void(int exit_code)> on_exit) {
    ProcessHandle handle;
    handle.id = id;

    stop(id);

    std::vector<std::string> storage;
    storage.reserve(args.size() + 1);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        storage.push_back(scrcpy_exe_);
    }
    storage.insert(storage.end(), args.begin(), args.end());

    std::vector<const char *> argv;
    argv.reserve(storage.size() + 1);
    for (const auto &s : storage) {
        argv.push_back(s.c_str());
    }
    argv.push_back(nullptr);

    auto proc = std::make_unique<ManagedProcess>();
    proc->id = id;
    proc->on_exit = std::move(on_exit);

    sc_pipe pout;
    sc_pipe perr;
    enum sc_process_result res =
        sc_process_execute_p(argv.data(), &proc->pid, 0, nullptr, &pout, &perr);
    if (res != SC_PROCESS_SUCCESS) {
        if (logs_) {
            if (res == SC_PROCESS_ERROR_MISSING_BINARY) {
                logs_->append_app("Could not find scrcpy executable: " + scrcpy_exe_);
            } else {
                logs_->append_app("Failed to start scrcpy for " + id);
            }
        }
        return handle;
    }

    proc->pout = pout;
    proc->perr = perr;
    proc->pout_valid = true;
    proc->perr_valid = true;
    proc->running = true;
    proc->stdout_thread = std::thread(&ProcessManager::reader_loop, this, proc.get(), pout, false);
    proc->stderr_thread = std::thread(&ProcessManager::reader_loop, this, proc.get(), perr, true);
    proc->wait_thread = std::thread(&ProcessManager::wait_loop, this, proc.get());

    handle.pid = proc->pid;

    if (logs_) {
        std::string cmd = scrcpy_exe_;
        for (const auto &a : args) {
            cmd += " ";
            cmd += a;
        }
        logs_->append_app("Started: " + cmd);
        logs_->append(id, "Session starting...");
    }

    std::lock_guard<std::mutex> lock(mutex_);
    processes_[id] = std::move(proc);
    return handle;
}

bool
ProcessManager::stop(const std::string &id) {
    std::unique_ptr<ManagedProcess> proc;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = processes_.find(id);
        if (it == processes_.end()) {
            return false;
        }
        proc = std::move(it->second);
        processes_.erase(it);
    }

    if (proc->running && proc->pid != SC_PROCESS_NONE) {
        sc_process_terminate(proc->pid);
    }
    if (proc->wait_thread.joinable()) {
        proc->wait_thread.join();
    }
    return true;
}

bool
ProcessManager::is_running(const std::string &id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = processes_.find(id);
    if (it == processes_.end()) {
        return false;
    }
    return it->second->running.load();
}

int
ProcessManager::get_exit_code(const std::string &id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = processes_.find(id);
    if (it == processes_.end()) {
        return -1;
    }
    return it->second->exit_code.load();
}

} // namespace scgui
