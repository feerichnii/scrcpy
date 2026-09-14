#include "gui/gui_app.h"

#include "imgui.h"

namespace scgui {

void
draw_devices_view(GuiApp &app) {
    auto devices = app.devices().devices();

    ImGui::Text("Connected devices: %zu", devices.size());
    ImGui::SameLine();
    if (ImGui::Button("Refresh")) {
        app.devices().refresh_now();
    }
    ImGui::SameLine();
    if (ImGui::Button("Restart ADB")) {
        app.logs().append_app("Restarting ADB server...");
        if (app.devices().restart_adb()) {
            app.logs().append_app("ADB server restarted");
        } else {
            app.logs().append_app("Failed to restart ADB server");
        }
    }

    size_t selected_count = 0;
    for (const auto &d : devices) {
        if (d.selected) {
            ++selected_count;
        }
    }
    ImGui::Text("%zu selected", selected_count);

    if (ImGui::Button("Start selected")) {
        app.sessions().start_selected(app.devices().selected_serials(true));
    }
    ImGui::SameLine();
    if (ImGui::Button("Start all")) {
        app.sessions().start_all(app.devices().all_ready_serials());
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop selected")) {
        app.sessions().stop_selected(app.devices().selected_serials(false));
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop all")) {
        app.sessions().stop_all();
    }
    ImGui::SameLine();
    if (ImGui::Button("Select all ready")) {
        app.devices().select_all_ready(true);
    }

    ImGui::Separator();

    if (ImGui::BeginTable("devices_table", 6,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                              ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY,
                          ImVec2(0, -1))) {
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 28);
        ImGui::TableSetupColumn("Device");
        ImGui::TableSetupColumn("Connection", ImGuiTableColumnFlags_WidthFixed, 90);
        ImGui::TableSetupColumn("ADB", ImGuiTableColumnFlags_WidthFixed, 120);
        ImGui::TableSetupColumn("Session", ImGuiTableColumnFlags_WidthFixed, 110);
        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 220);
        ImGui::TableHeadersRow();

        for (const auto &d : devices) {
            ImGui::PushID(d.serial.c_str());
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            bool selected = d.selected;
            if (ImGui::Checkbox("##sel", &selected)) {
                app.devices().set_selected(d.serial, selected);
            }

            ImGui::TableSetColumnIndex(1);
            DeviceSettings ds = app.settings().device(d.serial);
            std::string title = ds.alias.empty() ? (d.model.empty() ? d.serial : d.model) : ds.alias;
            ImGui::TextUnformatted(title.c_str());
            ImGui::TextDisabled("%s", d.serial.c_str());
            if (!d.model.empty() && !ds.alias.empty()) {
                ImGui::TextDisabled("%s", d.model.c_str());
            }

            ImGui::TableSetColumnIndex(2);
            ImGui::TextUnformatted(connection_type_name(d.connection_type));

            ImGui::TableSetColumnIndex(3);
            ImVec4 color = ImVec4(0.5f, 0.9f, 0.5f, 1.f);
            if (d.adb_state == AdbState::Unauthorized) {
                color = ImVec4(1.f, 0.8f, 0.2f, 1.f);
            } else if (d.adb_state == AdbState::Offline || d.adb_state == AdbState::Other) {
                color = ImVec4(1.f, 0.4f, 0.4f, 1.f);
            }
            ImGui::TextColored(color, "%s", adb_state_name(d.adb_state));

            ImGui::TableSetColumnIndex(4);
            ImGui::TextUnformatted(session_state_name(d.session_state));

            ImGui::TableSetColumnIndex(5);
            bool can_start = d.adb_state == AdbState::Device &&
                             (d.session_state == SessionState::Stopped ||
                              d.session_state == SessionState::Failed ||
                              d.session_state == SessionState::Disconnected);
            bool can_stop = d.session_state == SessionState::Running ||
                            d.session_state == SessionState::Starting;

            if (can_start) {
                if (ImGui::Button("Start")) {
                    app.sessions().start(d.serial);
                }
                ImGui::SameLine();
            }
            if (can_stop) {
                if (ImGui::Button("Stop")) {
                    app.sessions().stop(d.serial);
                }
                ImGui::SameLine();
            }
            if (d.session_state == SessionState::Failed) {
                if (ImGui::Button("Retry")) {
                    app.sessions().restart(d.serial);
                }
                ImGui::SameLine();
            }
            if (ImGui::Button("Settings")) {
                app.settings_device_serial() = d.serial;
            }

            if (!d.error_message.empty() &&
                (d.session_state == SessionState::Failed ||
                 d.session_state == SessionState::Disconnected)) {
                ImGui::TextWrapped("%s", d.error_message.c_str());
            }

            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}

} // namespace scgui
