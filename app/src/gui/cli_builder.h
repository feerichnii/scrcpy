#ifndef SC_GUI_CLI_BUILDER_H
#define SC_GUI_CLI_BUILDER_H

#include "gui/models.h"

#include <string>
#include <vector>

namespace scgui {

struct WindowRect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    bool valid = false;
};

std::vector<std::string>
build_scrcpy_args(const std::string &serial,
                  const DeviceSettings &settings,
                  const WindowRect &window);

} // namespace scgui

#endif
