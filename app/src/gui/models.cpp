#include "gui/models.h"

namespace scgui {

DeviceSettings
preset_settings(PresetId id) {
    DeviceSettings s;
    s.preset = id;
    switch (id) {
        case PresetId::Default:
            s.max_fps = 60;
            s.max_size = 0;
            s.audio = true;
            break;
        case PresetId::HighQuality:
            s.max_fps = 60;
            s.max_size = 1440;
            s.audio = true;
            break;
        case PresetId::LowBandwidth:
            s.max_fps = 30;
            s.max_size = 720;
            s.audio = true;
            break;
        case PresetId::Testing:
            s.max_fps = 60;
            s.max_size = 0;
            s.audio = false;
            break;
        case PresetId::Presentation:
            s.max_fps = 60;
            s.max_size = 1080;
            s.audio = true;
            s.always_on_top = true;
            break;
        case PresetId::Custom:
            break;
    }
    return s;
}

} // namespace scgui
