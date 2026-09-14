#include "gui/settings.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace scgui {

namespace {

std::string
escape_json(const std::string &s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    return out;
}

std::string
json_get_string(const std::string &json, const std::string &key, const std::string &def = {}) {
    std::string pattern = "\"" + key + "\"";
    size_t pos = json.find(pattern);
    if (pos == std::string::npos) {
        return def;
    }
    pos = json.find(':', pos);
    if (pos == std::string::npos) {
        return def;
    }
    pos = json.find('"', pos);
    if (pos == std::string::npos) {
        return def;
    }
    size_t end = pos + 1;
    std::string out;
    while (end < json.size()) {
        char c = json[end++];
        if (c == '\\' && end < json.size()) {
            out += json[end++];
            continue;
        }
        if (c == '"') {
            break;
        }
        out += c;
    }
    return out;
}

int
json_get_int(const std::string &json, const std::string &key, int def) {
    std::string pattern = "\"" + key + "\"";
    size_t pos = json.find(pattern);
    if (pos == std::string::npos) {
        return def;
    }
    pos = json.find(':', pos);
    if (pos == std::string::npos) {
        return def;
    }
    ++pos;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) {
        ++pos;
    }
    try {
        return std::stoi(json.substr(pos));
    } catch (...) {
        return def;
    }
}

bool
json_get_bool(const std::string &json, const std::string &key, bool def) {
    std::string pattern = "\"" + key + "\"";
    size_t pos = json.find(pattern);
    if (pos == std::string::npos) {
        return def;
    }
    pos = json.find(':', pos);
    if (pos == std::string::npos) {
        return def;
    }
    std::string rest = json.substr(pos + 1, 16);
    if (rest.find("true") != std::string::npos) {
        return true;
    }
    if (rest.find("false") != std::string::npos) {
        return false;
    }
    return def;
}

PresetId
preset_from_string(const std::string &s) {
    if (s == "high_quality") return PresetId::HighQuality;
    if (s == "low_bandwidth") return PresetId::LowBandwidth;
    if (s == "testing") return PresetId::Testing;
    if (s == "presentation") return PresetId::Presentation;
    if (s == "custom") return PresetId::Custom;
    return PresetId::Default;
}

std::string
preset_to_string(PresetId p) {
    switch (p) {
        case PresetId::HighQuality: return "high_quality";
        case PresetId::LowBandwidth: return "low_bandwidth";
        case PresetId::Testing: return "testing";
        case PresetId::Presentation: return "presentation";
        case PresetId::Custom: return "custom";
        default: return "default";
    }
}

VideoCodec
codec_from_string(const std::string &s) {
    if (s == "h265") return VideoCodec::H265;
    if (s == "av1") return VideoCodec::Av1;
    return VideoCodec::H264;
}

void
parse_settings_object(const std::string &obj, DeviceSettings &ds) {
    ds.alias = json_get_string(obj, "name", ds.alias);
    if (ds.alias.empty()) {
        ds.alias = json_get_string(obj, "alias", ds.alias);
    }
    ds.preset = preset_from_string(json_get_string(obj, "preset", "default"));
    ds.max_fps = json_get_int(obj, "max_fps", ds.max_fps);
    ds.max_size = json_get_int(obj, "max_size", ds.max_size);
    ds.audio = json_get_bool(obj, "audio", ds.audio);
    ds.video_codec = codec_from_string(json_get_string(obj, "video_codec", "h264"));
    ds.turn_screen_off = json_get_bool(obj, "turn_screen_off", ds.turn_screen_off);
    ds.always_on_top = json_get_bool(obj, "always_on_top", ds.always_on_top);
    ds.auto_start = json_get_bool(obj, "auto_start", ds.auto_start);
    ds.use_overrides = json_get_bool(obj, "use_overrides", true);
}

std::string
settings_to_json(const DeviceSettings &ds, bool include_alias) {
    std::ostringstream o;
    o << "{";
    bool first = true;
    auto field = [&](const std::string &k, const std::string &v, bool quoted) {
        if (!first) o << ",";
        first = false;
        o << "\"" << k << "\":";
        if (quoted) {
            o << "\"" << escape_json(v) << "\"";
        } else {
            o << v;
        }
    };
    if (include_alias && !ds.alias.empty()) {
        field("name", ds.alias, true);
    }
    field("preset", preset_to_string(ds.preset), true);
    field("max_fps", std::to_string(ds.max_fps), false);
    field("max_size", std::to_string(ds.max_size), false);
    field("audio", ds.audio ? "true" : "false", false);
    field("video_codec", video_codec_name(ds.video_codec), true);
    field("turn_screen_off", ds.turn_screen_off ? "true" : "false", false);
    field("always_on_top", ds.always_on_top ? "true" : "false", false);
    field("auto_start", ds.auto_start ? "true" : "false", false);
    field("use_overrides", ds.use_overrides ? "true" : "false", false);
    o << "}";
    return o.str();
}

} // namespace

std::string
SettingsManager::default_config_directory() {
#ifdef _WIN32
    const char *appdata = std::getenv("APPDATA");
    if (appdata && *appdata) {
        return (std::filesystem::path(appdata) / "scrcpy").string();
    }
    return "scrcpy";
#elif defined(__APPLE__)
    const char *home = std::getenv("HOME");
    if (home && *home) {
        return (std::filesystem::path(home) / "Library" / "Application Support" / "scrcpy").string();
    }
    return "scrcpy";
#else
    const char *xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg && *xdg) {
        return (std::filesystem::path(xdg) / "scrcpy").string();
    }
    const char *home = std::getenv("HOME");
    if (home && *home) {
        return (std::filesystem::path(home) / ".config" / "scrcpy").string();
    }
    return "scrcpy";
#endif
}

std::string
SettingsManager::config_directory() const {
    return default_config_directory();
}

std::string
SettingsManager::config_path() const {
    return (std::filesystem::path(default_config_directory()) / "scrcpy-gui.json").string();
}

bool
SettingsManager::load() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ifstream in(config_path());
    if (!in) {
        return false;
    }
    std::stringstream buffer;
    buffer << in.rdbuf();
    std::string json = buffer.str();

    size_t global_pos = json.find("\"global\"");
    if (global_pos != std::string::npos) {
        size_t brace = json.find('{', global_pos);
        size_t end = json.find('}', brace);
        if (brace != std::string::npos && end != std::string::npos) {
            std::string g = json.substr(brace, end - brace + 1);
            global_.max_fps = json_get_int(g, "max_fps", global_.max_fps);
            global_.max_size = json_get_int(g, "max_size", global_.max_size);
            global_.audio = json_get_bool(g, "audio", global_.audio);
            global_.video_codec = codec_from_string(json_get_string(g, "video_codec", "h264"));
            global_.turn_screen_off = json_get_bool(g, "turn_screen_off", global_.turn_screen_off);
            global_.always_on_top = json_get_bool(g, "always_on_top", global_.always_on_top);
            global_.stagger_ms = json_get_int(g, "stagger_ms", global_.stagger_ms);
            global_.max_simultaneous_startups =
                json_get_int(g, "max_simultaneous_startups", global_.max_simultaneous_startups);
            global_.auto_layout = json_get_bool(g, "auto_layout", global_.auto_layout);
            global_.layout_cols = json_get_int(g, "layout_cols", global_.layout_cols);
        }
    }

    size_t devices_pos = json.find("\"devices\"");
    if (devices_pos != std::string::npos) {
        size_t brace = json.find('{', devices_pos);
        if (brace != std::string::npos) {
            size_t i = brace + 1;
            while (i < json.size()) {
                while (i < json.size() && (json[i] == ' ' || json[i] == '\n' || json[i] == '\r' ||
                                           json[i] == '\t' || json[i] == ',')) {
                    ++i;
                }
                if (i >= json.size() || json[i] == '}') {
                    break;
                }
                if (json[i] != '"') {
                    break;
                }
                size_t key_start = ++i;
                while (i < json.size() && json[i] != '"') {
                    ++i;
                }
                if (i >= json.size()) {
                    break;
                }
                std::string serial = json.substr(key_start, i - key_start);
                ++i;
                size_t obj_start = json.find('{', i);
                if (obj_start == std::string::npos) {
                    break;
                }
                size_t obj_end = json.find('}', obj_start);
                if (obj_end == std::string::npos) {
                    break;
                }
                std::string obj = json.substr(obj_start, obj_end - obj_start + 1);
                DeviceSettings ds = preset_settings(PresetId::Default);
                parse_settings_object(obj, ds);
                devices_[serial] = ds;
                i = obj_end + 1;
            }
        }
    }
    return true;
}

bool
SettingsManager::save() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::error_code ec;
    std::filesystem::create_directories(default_config_directory(), ec);

    std::ostringstream o;
    o << "{\n  \"global\": {\n";
    o << "    \"max_fps\": " << global_.max_fps << ",\n";
    o << "    \"max_size\": " << global_.max_size << ",\n";
    o << "    \"audio\": " << (global_.audio ? "true" : "false") << ",\n";
    o << "    \"video_codec\": \"" << video_codec_name(global_.video_codec) << "\",\n";
    o << "    \"turn_screen_off\": " << (global_.turn_screen_off ? "true" : "false") << ",\n";
    o << "    \"always_on_top\": " << (global_.always_on_top ? "true" : "false") << ",\n";
    o << "    \"stagger_ms\": " << global_.stagger_ms << ",\n";
    o << "    \"max_simultaneous_startups\": " << global_.max_simultaneous_startups << ",\n";
    o << "    \"auto_layout\": " << (global_.auto_layout ? "true" : "false") << ",\n";
    o << "    \"layout_cols\": " << global_.layout_cols << "\n";
    o << "  },\n  \"devices\": {\n";

    bool first = true;
    for (const auto &kv : devices_) {
        if (!first) {
            o << ",\n";
        }
        first = false;
        o << "    \"" << escape_json(kv.first) << "\": " << settings_to_json(kv.second, true);
    }
    o << "\n  }\n}\n";

    std::ofstream out(config_path());
    if (!out) {
        return false;
    }
    out << o.str();
    return true;
}

GlobalSettings
SettingsManager::global() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return global_;
}

void
SettingsManager::set_global(const GlobalSettings &settings) {
    std::lock_guard<std::mutex> lock(mutex_);
    global_ = settings;
}

DeviceSettings
SettingsManager::device(const std::string &serial) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = devices_.find(serial);
    if (it == devices_.end()) {
        return DeviceSettings{};
    }
    return it->second;
}

void
SettingsManager::set_device(const std::string &serial, const DeviceSettings &settings) {
    std::lock_guard<std::mutex> lock(mutex_);
    devices_[serial] = settings;
}

DeviceSettings
SettingsManager::effective(const std::string &serial) const {
    std::lock_guard<std::mutex> lock(mutex_);
    DeviceSettings out;
    out.max_fps = global_.max_fps;
    out.max_size = global_.max_size;
    out.audio = global_.audio;
    out.video_codec = global_.video_codec;
    out.turn_screen_off = global_.turn_screen_off;
    out.always_on_top = global_.always_on_top;

    auto it = devices_.find(serial);
    if (it == devices_.end()) {
        return out;
    }

    const DeviceSettings &d = it->second;
    out.alias = d.alias;
    out.auto_start = d.auto_start;
    out.preset = d.preset;

    DeviceSettings from_preset = preset_settings(d.preset);
    if (d.preset != PresetId::Custom && d.preset != PresetId::Default) {
        out.max_fps = from_preset.max_fps;
        out.max_size = from_preset.max_size;
        out.audio = from_preset.audio;
        out.video_codec = from_preset.video_codec;
        out.turn_screen_off = from_preset.turn_screen_off;
        out.always_on_top = from_preset.always_on_top;
    }

    if (d.use_overrides || d.preset == PresetId::Custom) {
        out.max_fps = d.max_fps;
        out.max_size = d.max_size;
        out.audio = d.audio;
        out.video_codec = d.video_codec;
        out.turn_screen_off = d.turn_screen_off;
        out.always_on_top = d.always_on_top;
    }
    return out;
}

} // namespace scgui
