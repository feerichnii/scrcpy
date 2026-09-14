#include "gui/layout_manager.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>

namespace scgui {

void
LayoutManager::set_enabled(bool enabled) {
    enabled_ = enabled;
}

void
LayoutManager::set_columns(int cols) {
    columns_ = cols;
}

WindowRect
LayoutManager::allocate(size_t index, size_t total) const {
    WindowRect r;
    if (!enabled_ || total == 0) {
        return r;
    }

    const SDL_DisplayID display = SDL_GetPrimaryDisplay();
    const SDL_DisplayMode *mode = SDL_GetCurrentDisplayMode(display);
    int screen_w = 1920;
    int screen_h = 1080;
    if (mode) {
        screen_w = mode->w;
        screen_h = mode->h;
    }

    int margin = 24;
    int usable_w = std::max(320, screen_w - margin * 2);
    int usable_h = std::max(320, screen_h - margin * 2 - 40);

    int cols = columns_;
    if (cols <= 0) {
        cols = static_cast<int>(std::ceil(std::sqrt(static_cast<double>(total))));
        cols = std::max(1, cols);
    }
    int rows = static_cast<int>((total + static_cast<size_t>(cols) - 1) / static_cast<size_t>(cols));
    rows = std::max(1, rows);

    int cell_w = usable_w / cols;
    int cell_h = usable_h / rows;
    int gap = 8;

    int col = static_cast<int>(index % static_cast<size_t>(cols));
    int row = static_cast<int>(index / static_cast<size_t>(cols));

    r.x = margin + col * cell_w + gap / 2;
    r.y = margin + row * cell_h + gap / 2;
    r.width = std::max(200, cell_w - gap);
    r.height = std::max(200, cell_h - gap);
    r.valid = true;
    return r;
}

std::vector<WindowRect>
LayoutManager::allocate_all(size_t count) const {
    std::vector<WindowRect> out;
    out.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        out.push_back(allocate(i, count));
    }
    return out;
}

} // namespace scgui
