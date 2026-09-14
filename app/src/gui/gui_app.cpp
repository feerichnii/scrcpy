#include "gui/gui_app.h"

#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlrenderer3.h"

extern "C" {
#include "adb/adb.h"
#include "util/file.h"
#include "util/log.h"
}

#include <cstdlib>
#include <cstdio>

namespace scgui {

GuiApp::GuiApp() = default;

GuiApp::~GuiApp() {
    shutdown();
}

void
GuiApp::resolve_scrcpy_path() {
#ifdef PORTABLE
    char *local = sc_file_get_local_path("scrcpy");
    if (local) {
        processes_->set_scrcpy_executable(local);
        free(local);
        return;
    }
#endif
    // Prefer sibling binary next to scrcpy-gui
    char *sibling = sc_file_get_local_path("scrcpy");
    if (sibling) {
        if (sc_file_is_regular(sibling)) {
            processes_->set_scrcpy_executable(sibling);
            free(sibling);
            return;
        }
        free(sibling);
    }
    processes_->set_scrcpy_executable("scrcpy");
}

bool
GuiApp::init() {
    sc_set_log_level(SC_LOG_LEVEL_INFO);
    if (!sc_adb_init()) {
        return false;
    }

    logs_.set_log_directory(settings_.config_directory() + "/logs");
    settings_.load();
    logs_.append_app("scrcpy-gui starting");

    processes_ = std::make_unique<ProcessManager>(&logs_);
    resolve_scrcpy_path();
    sessions_ = std::make_unique<SessionManager>(processes_.get(), &settings_, &layout_, &logs_);
    devices_ = std::make_unique<DeviceManager>(sessions_.get(), &settings_);

    GlobalSettings gs = settings_.global();
    layout_.set_enabled(gs.auto_layout);
    layout_.set_columns(gs.layout_cols);

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        logs_.append_app(std::string("SDL_Init failed: ") + SDL_GetError());
        return false;
    }

    window_ = SDL_CreateWindow("scrcpy GUI", 1100, 720, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window_) {
        logs_.append_app(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
        return false;
    }

    renderer_ = SDL_CreateRenderer(window_, nullptr);
    if (!renderer_) {
        logs_.append_app(std::string("SDL_CreateRenderer failed: ") + SDL_GetError());
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForSDLRenderer(window_, renderer_);
    ImGui_ImplSDLRenderer3_Init(renderer_);

    devices_->refresh_now();
    return true;
}

void
GuiApp::render_frame() {
    devices_->tick();
    sessions_->tick();

    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::Begin("scrcpy GUI", nullptr, flags);
    ImGui::TextUnformatted("scrcpy GUI");
    ImGui::SameLine();
    ImGui::TextDisabled("multi-device manager");
    ImGui::Separator();

    if (ImGui::BeginTabBar("main_tabs")) {
        if (ImGui::BeginTabItem("Devices")) {
            draw_devices_view(*this);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Sessions")) {
            draw_sessions_view(*this);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Settings")) {
            draw_settings_view(*this);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Logs")) {
            draw_logs_view(*this);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();

    ImGui::Render();
    SDL_SetRenderDrawColor(renderer_, 30, 30, 34, 255);
    SDL_RenderClear(renderer_);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer_);
    SDL_RenderPresent(renderer_);
}

void
GuiApp::run() {
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                event.window.windowID == SDL_GetWindowID(window_)) {
                running = false;
            }
        }
        render_frame();
        SDL_Delay(16);
    }
}

void
GuiApp::shutdown() {
    if (sessions_) {
        sessions_->stop_all();
    }
    settings_.save();

    if (renderer_) {
        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    devices_.reset();
    sessions_.reset();
    processes_.reset();
    sc_adb_destroy();
    SDL_Quit();
}

} // namespace scgui
