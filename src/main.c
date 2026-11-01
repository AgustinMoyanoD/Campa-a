#include "game/app/app.h"

int main(void)
{
    AppContext app;
    if (!App_Init(&app, 1024, 768, "Estrategia Táctica - Nivel 1 Procedural")) {
        return 1;
    }

    while (!App_ShouldClose(&app)) {
        float dt = GetFrameTime();
        App_Update(&app, dt);
        App_Draw(&app);
    }

    App_Shutdown(&app);
    return 0;
}
