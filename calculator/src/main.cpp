#include "App.h"
#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")

int main() {
    App app;
    app.run();
    return 0;
}