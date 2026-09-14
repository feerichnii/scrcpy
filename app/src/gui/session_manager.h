#ifndef SC_GUI_SESSION_MANAGER_H
#define SC_GUI_SESSION_MANAGER_H

#include "gui/layout_manager.h"
#include "gui/log_manager.h"
#include "gui/models.h"
#include "gui/process_manager.h"
#include "gui/session/process_scrcpy_session.h"
#include "gui/settings.h"

#include <chrono>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace scgui {

class SessionManager {
public:
    SessionManager(ProcessManager *processes,
                   SettingsManager *settings,
                   LayoutManager *layout,
                   LogManager *logs);

    bool start(const std::string &serial);
    bool stop(const std::string &serial);
    bool restart(const std::string &serial);

    void start_selected(const std::vector<std::string> &serials);
    void start_all(const std::vector<std::string> &serials);
    void stop_selected(const std::vector<std::string> &serials);
    void stop_all();

    SessionState state(const std::string &serial) const;
    std::string error_message(const std::string &serial) const;

    void on_device_removed(const std::string &serial);
    void tick(); // process staggered start queue

    std::vector<std::string> active_serials() const;

private:
    struct PendingStart {
        std::string serial;
        std::chrono::steady_clock::time_point when;
    };

    ProcessManager *processes_;
    SettingsManager *settings_;
    LayoutManager *layout_;
    LogManager *logs_;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::unique_ptr<ProcessScrcpySession>> sessions_;
    std::deque<PendingStart> queue_;
    size_t layout_index_ = 0;
    size_t layout_total_ = 0;

    ProcessScrcpySession *ensure_session(const std::string &serial);
    bool start_now(const std::string &serial);
    void enqueue(const std::vector<std::string> &serials);
};

} // namespace scgui

#endif
