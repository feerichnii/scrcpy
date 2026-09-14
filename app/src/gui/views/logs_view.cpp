#include "gui/gui_app.h"

#include "imgui.h"

namespace scgui {

void
draw_logs_view(GuiApp &app) {
    auto channels = app.logs().channels();
    if (channels.empty()) {
        channels.push_back("Application");
    }

    if (ImGui::BeginCombo("Channel", app.selected_log_channel().c_str())) {
        for (const auto &ch : channels) {
            bool selected = ch == app.selected_log_channel();
            if (ImGui::Selectable(ch.c_str(), selected)) {
                app.selected_log_channel() = ch;
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::Separator();
    ImGui::BeginChild("log_scroll", ImVec2(0, 0), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
    auto lines = app.logs().lines(app.selected_log_channel());
    for (const auto &line : lines) {
        ImGui::TextUnformatted(line.c_str());
    }
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 8.f) {
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();
}

} // namespace scgui
