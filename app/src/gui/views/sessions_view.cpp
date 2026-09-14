#include "gui/gui_app.h"

#include "imgui.h"

namespace scgui {

void
draw_sessions_view(GuiApp &app) {
    auto devices = app.devices().devices();
    ImGui::TextUnformatted("Active and recent sessions");
    ImGui::Separator();

    if (ImGui::BeginTable("sessions_table", 5,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                              ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn("Device");
        ImGui::TableSetupColumn("Serial");
        ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed, 110);
        ImGui::TableSetupColumn("Connection", ImGuiTableColumnFlags_WidthFixed, 90);
        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 180);
        ImGui::TableHeadersRow();

        for (const auto &d : devices) {
            if (d.session_state == SessionState::Stopped && d.error_message.empty()) {
                continue;
            }
            ImGui::PushID(("sess_" + d.serial).c_str());
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            DeviceSettings ds = app.settings().device(d.serial);
            const char *name = !ds.alias.empty() ? ds.alias.c_str()
                                                 : (!d.model.empty() ? d.model.c_str() : d.serial.c_str());
            ImGui::TextUnformatted(name);

            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(d.serial.c_str());

            ImGui::TableSetColumnIndex(2);
            ImGui::TextUnformatted(session_state_name(d.session_state));

            ImGui::TableSetColumnIndex(3);
            ImGui::TextUnformatted(connection_type_name(d.connection_type));

            ImGui::TableSetColumnIndex(4);
            if (d.session_state == SessionState::Running ||
                d.session_state == SessionState::Starting) {
                if (ImGui::Button("Stop")) {
                    app.sessions().stop(d.serial);
                }
                ImGui::SameLine();
            }
            if (d.session_state == SessionState::Failed ||
                d.session_state == SessionState::Disconnected ||
                d.session_state == SessionState::Stopped) {
                if (d.adb_state == AdbState::Device && ImGui::Button("Start")) {
                    app.sessions().start(d.serial);
                }
            }
            if (!d.error_message.empty()) {
                ImGui::TextWrapped("%s", d.error_message.c_str());
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    if (ImGui::Button("Stop all sessions")) {
        app.sessions().stop_all();
    }
}

} // namespace scgui
