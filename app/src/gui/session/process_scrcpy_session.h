#ifndef SC_GUI_PROCESS_SCRCPY_SESSION_H
#define SC_GUI_PROCESS_SCRCPY_SESSION_H

#include "gui/cli_builder.h"
#include "gui/models.h"
#include "gui/process_manager.h"
#include "gui/session/iscrcpy_session.h"

#include <atomic>
#include <string>

namespace scgui {

class ProcessScrcpySession : public IScrcpySession {
public:
    ProcessScrcpySession(std::string serial,
                         ProcessManager *processes,
                         LogManager *logs);

    bool start(const DeviceSettings &settings, const WindowRect &window);
    bool start() override;
    void stop() override;
    SessionState state() const override;
    const std::string &serial() const override;

    void on_process_exit(int exit_code);
    void set_disconnected();
    int exit_code() const;
    const std::string &error_message() const;

private:
    std::string serial_;
    ProcessManager *processes_;
    LogManager *logs_;
    std::atomic<SessionState> state_{SessionState::Stopped};
    DeviceSettings pending_settings_;
    WindowRect pending_window_;
    int exit_code_ = 0;
    std::string error_message_;
};

} // namespace scgui

#endif
