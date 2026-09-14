#ifndef SC_GUI_LAYOUT_MANAGER_H
#define SC_GUI_LAYOUT_MANAGER_H

#include "gui/cli_builder.h"

#include <cstddef>
#include <vector>

namespace scgui {

class LayoutManager {
public:
    void set_enabled(bool enabled);
    void set_columns(int cols); // 0 = auto

    WindowRect allocate(size_t index, size_t total) const;
    std::vector<WindowRect> allocate_all(size_t count) const;

private:
    bool enabled_ = true;
    int columns_ = 0;
};

} // namespace scgui

#endif
