#ifndef SC_GUI_APP_H
#define SC_GUI_APP_H

#include "gui/device_manager.h"
#include "gui/layout_manager.h"
#include "gui/log_manager.h"
#include "gui/process_manager.h"
#include "gui/session_manager.h"
#include "gui/settings.h"

#include <SDL3/SDL.h>

#include <memory>
#include <string>

namespace scgui {

class GuiApp {
public:
    GuiApp();
    ~GuiApp();

    bool init();
    void run();
    void shutdown();

    DeviceManager &devices() { return *devices_; }
    SessionManager &sessions() { return *sessions_; }
    SettingsManager &settings() { return settings_; }
    LogManager &logs() { return logs_; }
    LayoutManager &layout() { return layout_; }

    std::string &selected_log_channel() { return selected_log_channel_; }
    std::string &settings_device_serial() { return settings_device_serial_; }

private:
    SDL_Window *window_ = nullptr;
    SDL_Renderer *renderer_ = nullptr;

    LogManager logs_;
    SettingsManager settings_;
    LayoutManager layout_;
    std::unique_ptr<ProcessManager> processes_;
    std::unique_ptr<SessionManager> sessions_;
    std::unique_ptr<DeviceManager> devices_;

    std::string selected_log_channel_ = "Application";
    std::string settings_device_serial_;

    void render_frame();
    void resolve_scrcpy_path();
};

void draw_devices_view(GuiApp &app);
void draw_sessions_view(GuiApp &app);
void draw_settings_view(GuiApp &app);
void draw_logs_view(GuiApp &app);

} // namespace scgui

#endif
