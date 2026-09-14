#ifndef SC_GUI_SETTINGS_H
#define SC_GUI_SETTINGS_H

#include "gui/models.h"

#include <mutex>
#include <string>
#include <unordered_map>

namespace scgui {

class SettingsManager {
public:
    bool load();
    bool save() const;

    GlobalSettings global() const;
    void set_global(const GlobalSettings &settings);

    DeviceSettings device(const std::string &serial) const;
    void set_device(const std::string &serial, const DeviceSettings &settings);

    DeviceSettings effective(const std::string &serial) const;

    std::string config_path() const;
    std::string config_directory() const;

private:
    mutable std::mutex mutex_;
    GlobalSettings global_;
    std::unordered_map<std::string, DeviceSettings> devices_;

    static std::string default_config_directory();
};

} // namespace scgui

#endif
