#include "gui/cli_builder.h"

#include <sstream>

namespace scgui {

std::vector<std::string>
build_scrcpy_args(const std::string &serial,
                  const DeviceSettings &settings,
                  const WindowRect &window) {
    std::vector<std::string> args;
    args.push_back("-s");
    args.push_back(serial);

    if (settings.max_fps > 0) {
        args.push_back("--max-fps=" + std::to_string(settings.max_fps));
    }
    if (settings.max_size > 0) {
        args.push_back("--max-size=" + std::to_string(settings.max_size));
    }
    if (!settings.audio) {
        args.push_back("--no-audio");
    }
    args.push_back(std::string("--video-codec=") + video_codec_name(settings.video_codec));
    if (settings.turn_screen_off) {
        args.push_back("--turn-screen-off");
    }
    if (settings.always_on_top) {
        args.push_back("--always-on-top");
    }
    if (!settings.alias.empty()) {
        args.push_back("--window-title=" + settings.alias);
    }
    if (window.valid) {
        args.push_back("--window-x=" + std::to_string(window.x));
        args.push_back("--window-y=" + std::to_string(window.y));
        if (window.width > 0) {
            args.push_back("--window-width=" + std::to_string(window.width));
        }
        if (window.height > 0) {
            args.push_back("--window-height=" + std::to_string(window.height));
        }
    }
    return args;
}

} // namespace scgui
