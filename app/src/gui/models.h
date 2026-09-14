#ifndef SC_GUI_MODELS_H
#define SC_GUI_MODELS_H

#include <cstdint>
#include <string>
#include <vector>

namespace scgui {

enum class ConnectionType {
    Usb,
    TcpIp,
    Emulator,
};

enum class AdbState {
    Device,
    Unauthorized,
    Offline,
    Disconnected,
    Other,
};

enum class SessionState {
    Stopped,
    Starting,
    Running,
    Stopping,
    Failed,
    Disconnected,
};

enum class VideoCodec {
    H264,
    H265,
    Av1,
};

enum class PresetId {
    Default,
    HighQuality,
    LowBandwidth,
    Testing,
    Presentation,
    Custom,
};

struct DeviceSettings {
    std::string alias;
    PresetId preset = PresetId::Default;
    int max_fps = 60;
    int max_size = 0; // 0 = auto
    bool audio = true;
    VideoCodec video_codec = VideoCodec::H264;
    bool turn_screen_off = false;
    bool always_on_top = false;
    bool auto_start = false;
    bool use_overrides = false;
};

struct GlobalSettings {
    int max_fps = 60;
    int max_size = 0;
    bool audio = true;
    VideoCodec video_codec = VideoCodec::H264;
    bool turn_screen_off = false;
    bool always_on_top = false;
    int stagger_ms = 200;
    int max_simultaneous_startups = 4;
    bool auto_layout = true;
    int layout_cols = 0; // 0 = auto
};

struct Device {
    std::string serial;
    std::string model;
    ConnectionType connection_type = ConnectionType::Usb;
    AdbState adb_state = AdbState::Other;
    SessionState session_state = SessionState::Stopped;
    bool selected = false;
    std::string error_message;
    int64_t started_at_ms = 0;
    int exit_code = 0;
};

inline const char *
connection_type_name(ConnectionType t) {
    switch (t) {
        case ConnectionType::Usb: return "USB";
        case ConnectionType::TcpIp: return "TCP/IP";
        case ConnectionType::Emulator: return "Emulator";
    }
    return "?";
}

inline const char *
adb_state_name(AdbState s) {
    switch (s) {
        case AdbState::Device: return "Connected";
        case AdbState::Unauthorized: return "Unauthorized";
        case AdbState::Offline: return "Offline";
        case AdbState::Disconnected: return "Disconnected";
        case AdbState::Other: return "Unknown";
    }
    return "?";
}

inline const char *
session_state_name(SessionState s) {
    switch (s) {
        case SessionState::Stopped: return "Stopped";
        case SessionState::Starting: return "Starting";
        case SessionState::Running: return "Running";
        case SessionState::Stopping: return "Stopping";
        case SessionState::Failed: return "Failed";
        case SessionState::Disconnected: return "Disconnected";
    }
    return "?";
}

inline const char *
preset_name(PresetId p) {
    switch (p) {
        case PresetId::Default: return "Default";
        case PresetId::HighQuality: return "High Quality";
        case PresetId::LowBandwidth: return "Low Bandwidth";
        case PresetId::Testing: return "Testing";
        case PresetId::Presentation: return "Presentation";
        case PresetId::Custom: return "Custom";
    }
    return "Default";
}

inline const char *
video_codec_name(VideoCodec c) {
    switch (c) {
        case VideoCodec::H264: return "h264";
        case VideoCodec::H265: return "h265";
        case VideoCodec::Av1: return "av1";
    }
    return "h264";
}

DeviceSettings
preset_settings(PresetId id);

} // namespace scgui

#endif
