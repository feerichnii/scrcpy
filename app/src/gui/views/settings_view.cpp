#include "gui/gui_app.h"

#include "imgui.h"

#include <algorithm>
#include <cstdio>

namespace scgui {

namespace {

bool
combo_preset(const char *label, PresetId *preset) {
    const char *items[] = {
        "Default", "High Quality", "Low Bandwidth", "Testing", "Presentation", "Custom",
    };
    int current = static_cast<int>(*preset);
    if (ImGui::Combo(label, &current, items, IM_ARRAYSIZE(items))) {
        *preset = static_cast<PresetId>(current);
        return true;
    }
    return false;
}

bool
combo_codec(const char *label, VideoCodec *codec) {
    const char *items[] = {"H.264", "H.265", "AV1"};
    int current = static_cast<int>(*codec);
    if (ImGui::Combo(label, &current, items, IM_ARRAYSIZE(items))) {
        *codec = static_cast<VideoCodec>(current);
        return true;
    }
    return false;
}

void
edit_common_fields(DeviceSettings &ds) {
    int fps = ds.max_fps;
    if (ImGui::InputInt("FPS", &fps)) {
        ds.max_fps = std::max(1, std::min(fps, 120));
    }

    const char *sizes[] = {"Auto", "720p", "1080p", "1440p"};
    int size_idx = 0;
    if (ds.max_size == 720) size_idx = 1;
    else if (ds.max_size == 1080) size_idx = 2;
    else if (ds.max_size == 1440) size_idx = 3;
    if (ImGui::Combo("Resolution", &size_idx, sizes, IM_ARRAYSIZE(sizes))) {
        switch (size_idx) {
            case 1: ds.max_size = 720; break;
            case 2: ds.max_size = 1080; break;
            case 3: ds.max_size = 1440; break;
            default: ds.max_size = 0; break;
        }
    }

    ImGui::Checkbox("Audio", &ds.audio);
    combo_codec("Video codec", &ds.video_codec);
    ImGui::Checkbox("Turn device screen off", &ds.turn_screen_off);
    ImGui::Checkbox("Always on top", &ds.always_on_top);
}

} // namespace

void
draw_settings_view(GuiApp &app) {
    if (ImGui::BeginTabBar("settings_tabs")) {
        if (ImGui::BeginTabItem("Global")) {
            GlobalSettings gs = app.settings().global();
            bool changed = false;

            int fps = gs.max_fps;
            if (ImGui::InputInt("Default FPS", &fps)) {
                gs.max_fps = std::max(1, std::min(fps, 120));
                changed = true;
            }

            const char *sizes[] = {"Auto", "720p", "1080p", "1440p"};
            int size_idx = 0;
            if (gs.max_size == 720) size_idx = 1;
            else if (gs.max_size == 1080) size_idx = 2;
            else if (gs.max_size == 1440) size_idx = 3;
            if (ImGui::Combo("Default resolution", &size_idx, sizes, IM_ARRAYSIZE(sizes))) {
                switch (size_idx) {
                    case 1: gs.max_size = 720; break;
                    case 2: gs.max_size = 1080; break;
                    case 3: gs.max_size = 1440; break;
                    default: gs.max_size = 0; break;
                }
                changed = true;
            }

            changed |= ImGui::Checkbox("Audio enabled", &gs.audio);
            {
                VideoCodec codec = gs.video_codec;
                if (combo_codec("Video codec", &codec)) {
                    gs.video_codec = codec;
                    changed = true;
                }
            }
            changed |= ImGui::Checkbox("Turn screen off", &gs.turn_screen_off);
            changed |= ImGui::Checkbox("Always on top", &gs.always_on_top);

            ImGui::Separator();
            ImGui::TextUnformatted("Multi-device startup");
            changed |= ImGui::InputInt("Stagger delay (ms)", &gs.stagger_ms);
            changed |= ImGui::InputInt("Max simultaneous startups", &gs.max_simultaneous_startups);
            changed |= ImGui::Checkbox("Auto window layout", &gs.auto_layout);
            changed |= ImGui::InputInt("Layout columns (0=auto)", &gs.layout_cols);

            if (changed) {
                app.settings().set_global(gs);
                app.layout().set_enabled(gs.auto_layout);
                app.layout().set_columns(gs.layout_cols);
            }

            if (ImGui::Button("Save settings")) {
                if (app.settings().save()) {
                    app.logs().append_app("Settings saved to " + app.settings().config_path());
                } else {
                    app.logs().append_app("Failed to save settings");
                }
            }
            ImGui::TextDisabled("%s", app.settings().config_path().c_str());
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Per-device")) {
            auto devices = app.devices().devices();
            if (devices.empty()) {
                ImGui::TextUnformatted("No devices connected.");
            } else {
                if (app.settings_device_serial().empty()) {
                    app.settings_device_serial() = devices.front().serial;
                }

                if (ImGui::BeginCombo("Device", app.settings_device_serial().c_str())) {
                    for (const auto &d : devices) {
                        bool selected = d.serial == app.settings_device_serial();
                        std::string label = d.model.empty() ? d.serial : (d.model + " (" + d.serial + ")");
                        if (ImGui::Selectable(label.c_str(), selected)) {
                            app.settings_device_serial() = d.serial;
                        }
                        if (selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                const std::string &serial = app.settings_device_serial();
                DeviceSettings ds = app.settings().device(serial);
                if (ds.preset == PresetId::Default && !ds.use_overrides) {
                    // keep as-is
                }

                char alias[128];
                std::snprintf(alias, sizeof(alias), "%s", ds.alias.c_str());
                if (ImGui::InputText("Alias", alias, sizeof(alias))) {
                    ds.alias = alias;
                }

                if (combo_preset("Preset", &ds.preset)) {
                    if (ds.preset != PresetId::Custom) {
                        DeviceSettings preset = preset_settings(ds.preset);
                        ds.max_fps = preset.max_fps;
                        ds.max_size = preset.max_size;
                        ds.audio = preset.audio;
                        ds.video_codec = preset.video_codec;
                        ds.turn_screen_off = preset.turn_screen_off;
                        ds.always_on_top = preset.always_on_top;
                        ds.use_overrides = false;
                    } else {
                        ds.use_overrides = true;
                    }
                }

                ImGui::Checkbox("Override preset values", &ds.use_overrides);
                if (ds.use_overrides || ds.preset == PresetId::Custom) {
                    edit_common_fields(ds);
                    ds.preset = PresetId::Custom;
                    ds.use_overrides = true;
                }

                ImGui::Checkbox("Auto-start when connected", &ds.auto_start);

                if (ImGui::Button("Apply device settings")) {
                    app.settings().set_device(serial, ds);
                    app.settings().save();
                    app.logs().append_app("Saved settings for " + serial);
                }
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

} // namespace scgui
