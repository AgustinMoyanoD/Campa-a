#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "engine/core/hex_grid.h"
#include "engine/core/pathfinding.h"
#include "engine/core/config_parser.h"
#include "engine/core/rng.h"
#include "engine/core/day_clock.h"
#include "engine/presentation/camera_controller.h"
#include "engine/presentation/tween.h"
#include "engine/presentation/sequence.h"
#include "engine/presentation/screen_fx.h"
#include "engine/presentation/audio_engine.h"
#include "engine/presentation/asset_cache.h"
#include "engine/ui/ui_theme.h"
#include "engine/ui/ui_skin.h"
#include "engine/ui/ui_widgets.h"
#include "engine/ui/glossary.h"
#include "engine/systems/effect_system.h"

// Callback de prueba para A*
static bool DummyPassable(HexCoord coord, void* userData) {
    (void)userData;
    // Bloquear (1, 1) como obstáculo
    if (coord.q == 1 && coord.r == 1) return false;
    return true;
}

static float DummyCost(HexCoord from, HexCoord to, void* userData) {
    (void)from; (void)to; (void)userData;
    return 1.0f;
}

int main(void) {
    printf("============================================================\n");
    printf("TEST STANDALONE: MOTOR REUTILIZABLE (CERO DEPENDENCIAS DE JUEGO)\n");
    printf("============================================================\n\n");

    // 1. Hex Grid Math
    printf("[1/6] Verificando matematicas de cuadricula hexagonal...\n");
    HexCoord c0 = HexCoord_Create(0, 0);
    HexCoord c1 = HexCoord_Create(2, -1);
    int dist = HexCoord_Distance(c0, c1);
    assert(dist == 2);

    HexCoord nb = HexCoord_GetNeighbor(c0, HEX_RIGHT);
    assert(nb.q == 1 && nb.r == 0);

    Vector2 pix = HexToPixel(c0, 32.0f);
    HexCoord back = PixelToHex(pix, 32.0f);
    assert(HexCoord_Equal(c0, back));
    printf("  -> OK: Distancia, vecinos y proyeccion isometrica validada.\n");

    // 2. Generic Pathfinding A*
    printf("[2/6] Verificando algoritmo A* generico por callbacks...\n");
    PathfindParams params;
    params.start = HexCoord_Create(0, 0);
    params.goal = HexCoord_Create(2, 2);
    params.isPassable = DummyPassable;
    params.getCost = DummyCost;
    params.heuristic = NULL;
    params.userData = NULL;
    params.maxSearchNodes = 512;

    HexCoord path[32];
    float cost = 0.0f;
    int len = Pathfind_AStar(&params, path, 32, &cost);
    assert(len > 0);
    assert(HexCoord_Equal(path[len - 1], params.goal));
    // Comprobar que no atraviesa el obstáculo (1, 1)
    for (int i = 0; i < len; i++) {
        assert(!(path[i].q == 1 && path[i].r == 1));
    }
    printf("  -> OK: A* resolvio ruta sorteando obstaculo (longitud=%d, coste=%.1f).\n", len, cost);

    // 3. Generic Config Parser
    printf("[3/6] Verificando lector generico INI / CFG...\n");
    const char* sampleIni =
        "[SECTION_A]\n"
        "Name=Antigravity\n"
        "Speed=42.5\n"
        "Count=100\n"
        "Enabled=true\n"
        "Composite=food:1.5 water:0.8 stealth:2\n"
        "; Comentario ignorado\n"
        "[SECTION_B]\n"
        "Title=EngineCore\n";

    ConfigFile cfg;
    bool loadOk = Config_LoadString(&cfg, sampleIni);
    assert(loadOk);
    assert(strcmp(Config_GetString(&cfg, "SECTION_A", "Name", ""), "Antigravity") == 0);
    assert(fabsf(Config_GetFloat(&cfg, "SECTION_A", "Speed", 0.0f) - 42.5f) < 0.001f);
    assert(Config_GetInt(&cfg, "SECTION_A", "Count", 0) == 100);
    assert(Config_GetBool(&cfg, "SECTION_A", "Enabled", false) == true);

    char subVal[32];
    assert(Config_GetSubValue(Config_GetString(&cfg, "SECTION_A", "Composite", ""), "water", subVal, sizeof(subVal)));
    assert(fabsf((float)atof(subVal) - 0.8f) < 0.001f);
    assert(Config_GetSubInt(Config_GetString(&cfg, "SECTION_A", "Composite", ""), "stealth", 0) == 2);
    printf("  -> OK: Parseo de secciones, claves tipadas y pares compuestos exitoso.\n");

    // 4. RNG Determinista y Reloj
    printf("[4/6] Verificando RNG PCG32 y Reloj de franjas...\n");
    Rng rng;
    Rng_Init(&rng, 12345);
    int r1 = Rng_Range(&rng, 1, 6);
    int r2 = Rng_Range(&rng, 1, 6);
    assert(r1 >= 1 && r1 <= 6);
    assert(r2 >= 1 && r2 <= 6);

    DayClock clock;
    DayClock_Init(&clock, 1, TIME_MORNING);
    assert(clock.currentDay == 1 && clock.timeOfDay == TIME_MORNING);
    DayClock_AdvanceSlot(&clock);
    assert(clock.timeOfDay == TIME_MIDDAY);
    DayClock_AdvanceSlot(&clock); // AFTERNOON
    DayClock_AdvanceSlot(&clock); // NIGHT
    DayClock_AdvanceSlot(&clock); // MORNING next day
    assert(clock.currentDay == 2 && clock.dayEnded);
    printf("  -> OK: RNG y Reloj temporal operativos.\n");

    // 5. Tweens y Easing
    printf("[5/6] Verificando interpolacion y tweening...\n");
    float val = 0.0f;
    Tween_Start(&val, 100.0f, 1.0f, Ease_Linear);
    Tween_Update(0.5f);
    assert(fabsf(val - 50.0f) < 1.0f);
    Tween_Update(0.5f);
    assert(fabsf(val - 100.0f) < 0.001f);
    printf("  -> OK: Sistema de Tween e interpolacion lineal verificado.\n");

    // 6. Effect System Parser
    printf("[6/6] Verificando parseador de efectos genéricos...\n");
    EffectDef eff;
    bool effOk = Effect_ParseString("gold:+50", &eff);
    assert(effOk);
    assert(eff.type == EFFECT_GOLD);
    assert(eff.value == 50.0f);
    printf("  -> OK: Parseo generico de efectos verificado.\n\n");

    printf("============================================================\n");
    printf("¡MOTOR TOTALMENTE DESACOPLADO Y OPERATIVO AL 100%%!\n");
    printf("============================================================\n");
    return 0;
}
