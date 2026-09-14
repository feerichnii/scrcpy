#ifndef SC_GUI_DEVICE_MANAGER_H
#define SC_GUI_DEVICE_MANAGER_H

#include "gui/models.h"
#include "gui/session_manager.h"
#include "gui/settings.h"

#include <chrono>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

namespace scgui {

class DeviceManager {
public:
    DeviceManager(SessionManager *sessions, SettingsManager *settings);

    void set_poll_interval_ms(int ms);
    void refresh_now();
    void tick();

    std::vector<Device> devices() const;
    void set_selected(const std::string &serial, bool selected);
    void select_all_ready(bool selected);

    std::vector<std::string> selected_serials(bool ready_only = true) const;
    std::vector<std::string> all_ready_serials() const;

    bool restart_adb();

private:
    SessionManager *sessions_;
    SettingsManager *settings_;
    mutable std::mutex mutex_;
    std::vector<Device> devices_;
    std::unordered_set<std::string> selected_;
    std::chrono::steady_clock::time_point last_poll_{};
    int poll_interval_ms_ = 1500;
    bool force_refresh_ = true;

    void poll_devices();
};

} // namespace scgui

#endif
