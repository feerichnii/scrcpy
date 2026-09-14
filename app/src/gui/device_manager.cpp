#include "gui/device_manager.h"

#include <cstring>

extern "C" {
#include "adb/adb.h"
#include "adb/adb_device.h"
#include "util/intr.h"
}

namespace scgui {

namespace {

AdbState
parse_adb_state(const char *state) {
    if (!state) {
        return AdbState::Other;
    }
    if (!std::strcmp(state, "device")) {
        return AdbState::Device;
    }
    if (!std::strcmp(state, "unauthorized")) {
        return AdbState::Unauthorized;
    }
    if (!std::strcmp(state, "offline")) {
        return AdbState::Offline;
    }
    return AdbState::Other;
}

ConnectionType
parse_connection_type(const char *serial) {
    switch (sc_adb_device_get_type(serial)) {
        case SC_ADB_DEVICE_TYPE_USB:
            return ConnectionType::Usb;
        case SC_ADB_DEVICE_TYPE_TCPIP:
            return ConnectionType::TcpIp;
        case SC_ADB_DEVICE_TYPE_EMULATOR:
            return ConnectionType::Emulator;
    }
    return ConnectionType::Usb;
}

} // namespace

DeviceManager::DeviceManager(SessionManager *sessions, SettingsManager *settings)
    : sessions_(sessions)
    , settings_(settings) {
}

void
DeviceManager::set_poll_interval_ms(int ms) {
    poll_interval_ms_ = ms;
}

void
DeviceManager::refresh_now() {
    force_refresh_ = true;
}

void
DeviceManager::tick() {
    auto now = std::chrono::steady_clock::now();
    if (!force_refresh_) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_poll_).count();
        if (elapsed < poll_interval_ms_) {
            return;
        }
    }
    force_refresh_ = false;
    last_poll_ = now;
    poll_devices();
}

void
DeviceManager::poll_devices() {
    struct sc_intr intr;
    if (!sc_intr_init(&intr)) {
        return;
    }

    struct sc_vec_adb_devices vec = SC_VECTOR_INITIALIZER;
    bool ok = sc_adb_list_devices(&intr, SC_ADB_SILENT, &vec);
    sc_intr_destroy(&intr);
    if (!ok) {
        return;
    }

    std::unordered_set<std::string> seen;
    std::vector<Device> next;
    next.reserve(vec.size);
    std::vector<std::string> removed;
    std::vector<std::string> newly_ready;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (size_t i = 0; i < vec.size; ++i) {
            const struct sc_adb_device &d = vec.data[i];
            if (!d.serial) {
                continue;
            }
            Device device;
            device.serial = d.serial;
            device.model = d.model ? d.model : "";
            device.connection_type = parse_connection_type(d.serial);
            device.adb_state = parse_adb_state(d.state);
            device.selected = selected_.count(device.serial) > 0;
            device.session_state = sessions_->state(device.serial);
            device.error_message = sessions_->error_message(device.serial);
            seen.insert(device.serial);
            next.push_back(std::move(device));
        }

        for (const auto &old : devices_) {
            if (!seen.count(old.serial)) {
                removed.push_back(old.serial);
                selected_.erase(old.serial);
            }
        }

        for (const auto &device : next) {
            if (device.adb_state != AdbState::Device) {
                continue;
            }
            bool was_ready = false;
            for (const auto &old : devices_) {
                if (old.serial == device.serial && old.adb_state == AdbState::Device) {
                    was_ready = true;
                    break;
                }
            }
            if (!was_ready) {
                newly_ready.push_back(device.serial);
            }
        }

        devices_ = std::move(next);
    }

    sc_adb_devices_destroy(&vec);

    for (const auto &serial : removed) {
        sessions_->on_device_removed(serial);
    }

    for (const auto &serial : newly_ready) {
        DeviceSettings ds = settings_->device(serial);
        if (ds.auto_start) {
            SessionState st = sessions_->state(serial);
            if (st == SessionState::Stopped || st == SessionState::Failed ||
                st == SessionState::Disconnected) {
                sessions_->start(serial);
            }
        }
    }
}

std::vector<Device>
DeviceManager::devices() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Device> out = devices_;
    for (auto &d : out) {
        d.session_state = sessions_->state(d.serial);
        d.error_message = sessions_->error_message(d.serial);
        d.selected = selected_.count(d.serial) > 0;
    }
    return out;
}

void
DeviceManager::set_selected(const std::string &serial, bool selected) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (selected) {
        selected_.insert(serial);
    } else {
        selected_.erase(serial);
    }
    for (auto &d : devices_) {
        if (d.serial == serial) {
            d.selected = selected;
        }
    }
}

void
DeviceManager::select_all_ready(bool selected) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto &d : devices_) {
        if (d.adb_state == AdbState::Device) {
            d.selected = selected;
            if (selected) {
                selected_.insert(d.serial);
            } else {
                selected_.erase(d.serial);
            }
        }
    }
}

std::vector<std::string>
DeviceManager::selected_serials(bool ready_only) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> out;
    for (const auto &d : devices_) {
        if (!selected_.count(d.serial)) {
            continue;
        }
        if (ready_only && d.adb_state != AdbState::Device) {
            continue;
        }
        out.push_back(d.serial);
    }
    return out;
}

std::vector<std::string>
DeviceManager::all_ready_serials() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> out;
    for (const auto &d : devices_) {
        if (d.adb_state == AdbState::Device) {
            out.push_back(d.serial);
        }
    }
    return out;
}

bool
DeviceManager::restart_adb() {
    struct sc_intr intr;
    if (!sc_intr_init(&intr)) {
        return false;
    }
    bool ok = sc_adb_restart_server(&intr, 0);
    sc_intr_destroy(&intr);
    refresh_now();
    return ok;
}

} // namespace scgui
