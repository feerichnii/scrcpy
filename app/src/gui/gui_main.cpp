#include "gui/gui_app.h"

#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#endif

int
main(int argc, char **argv) {
    (void) argc;
    (void) argv;

    scgui::GuiApp app;
    if (!app.init()) {
        return 1;
    }
    app.run();
    app.shutdown();
    return 0;
}

#ifdef _WIN32
int WINAPI
WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return main(__argc, __argv);
}
#endif
