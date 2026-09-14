#include "gui/session/process_scrcpy_session.h"

namespace scgui {

ProcessScrcpySession::ProcessScrcpySession(std::string serial,
                                           ProcessManager *processes,
                                           LogManager *logs)
    : serial_(std::move(serial))
    , processes_(processes)
    , logs_(logs) {
}

bool
ProcessScrcpySession::start(const DeviceSettings &settings, const WindowRect &window) {
    pending_settings_ = settings;
    pending_window_ = window;
    return start();
}

bool
ProcessScrcpySession::start() {
    if (state_ == SessionState::Starting || state_ == SessionState::Running) {
        return true;
    }
    state_ = SessionState::Starting;
    error_message_.clear();

    auto args = build_scrcpy_args(serial_, pending_settings_, pending_window_);
    auto handle = processes_->start(serial_, args, [this](int code) {
        on_process_exit(code);
    });
    if (handle.pid == SC_PROCESS_NONE) {
        state_ = SessionState::Failed;
        error_message_ = logs_ ? logs_->friendly_error_from_logs(serial_)
                               : "Failed to start scrcpy process";
        return false;
    }
    state_ = SessionState::Running;
    return true;
}

void
ProcessScrcpySession::stop() {
    if (state_ == SessionState::Stopped) {
        return;
    }
    state_ = SessionState::Stopping;
    processes_->stop(serial_);
    state_ = SessionState::Stopped;
}

SessionState
ProcessScrcpySession::state() const {
    return state_.load();
}

const std::string &
ProcessScrcpySession::serial() const {
    return serial_;
}

void
ProcessScrcpySession::on_process_exit(int exit_code) {
    exit_code_ = exit_code;
    if (state_ == SessionState::Stopping) {
        state_ = SessionState::Stopped;
        return;
    }
    if (exit_code == 0) {
        state_ = SessionState::Stopped;
    } else {
        state_ = SessionState::Failed;
        if (logs_) {
            error_message_ = logs_->friendly_error_from_logs(serial_);
        }
    }
}

void
ProcessScrcpySession::set_disconnected() {
    if (state_ == SessionState::Running || state_ == SessionState::Starting) {
        state_ = SessionState::Disconnected;
        processes_->stop(serial_);
        error_message_ = "USB disconnected";
    }
}

int
ProcessScrcpySession::exit_code() const {
    return exit_code_;
}

const std::string &
ProcessScrcpySession::error_message() const {
    return error_message_;
}

} // namespace scgui
