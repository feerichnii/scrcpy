#ifndef SC_GUI_PROCESS_MANAGER_H
#define SC_GUI_PROCESS_MANAGER_H

#include "gui/log_manager.h"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

extern "C" {
#include "util/process.h"
}

namespace scgui {

struct ProcessHandle {
    std::string id;
    sc_pid pid = SC_PROCESS_NONE;
};

class ProcessManager {
public:
    explicit ProcessManager(LogManager *logs);
    ~ProcessManager();

    ProcessManager(const ProcessManager &) = delete;
    ProcessManager &operator=(const ProcessManager &) = delete;

    void set_scrcpy_executable(const std::string &path);

    ProcessHandle start(const std::string &id,
                        const std::vector<std::string> &args,
                        std::function<void(int exit_code)> on_exit = {});

    bool stop(const std::string &id);
    bool is_running(const std::string &id) const;
    int get_exit_code(const std::string &id) const;

private:
    struct ManagedProcess {
        std::string id;
        sc_pid pid = SC_PROCESS_NONE;
        sc_pipe pout{};
        sc_pipe perr{};
        std::thread stdout_thread;
        std::thread stderr_thread;
        std::thread wait_thread;
        std::atomic<bool> running{false};
        std::atomic<int> exit_code{0};
        std::function<void(int)> on_exit;
        bool pout_valid = false;
        bool perr_valid = false;
    };

    LogManager *logs_;
    std::string scrcpy_exe_ = "scrcpy";
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::unique_ptr<ManagedProcess>> processes_;

    void reader_loop(ManagedProcess *proc, sc_pipe pipe, bool is_stderr);
    void wait_loop(ManagedProcess *proc);
};

} // namespace scgui

#endif
