#include "views/enemy_view.h"
#include "views/map_view.h"
#include <stdio.h>

void EnemyView_DrawOnMap(const EnemySquad* enemy, Vector2 visualPos, Vector2 origin, const GameAssets* assets, bool isHovered, float hexRadius) {
    if (!enemy || !enemy->isActive) return;
    
    Vector2 pos = visualPos;
    pos.x += origin.x;
    pos.y += origin.y - 10.0f; // Ajuste vertical basico para perspectiva
    
    // Rango de amenaza al pasar el mouse (8.6)
    if (isHovered) {
        int sense = (enemy->senseRadius > 0) ? enemy->senseRadius : 3;
        float threatWorldRadius = (float)sense * hexRadius * 1.5f;
        DrawCircleLines(pos.x, pos.y, threatWorldRadius, Fade(RED, 0.7f));
        DrawCircle(pos.x, pos.y, threatWorldRadius, Fade(RED, 0.12f));
    }

    // Dibujar el marcador del escuadrón
    if (assets && assets->enemyToken.id != 0) {
        float scale = 30.0f / assets->enemyToken.width;
        DrawTextureEx(assets->enemyToken, (Vector2){pos.x - 15.0f, pos.y - 15.0f}, 0.0f, scale, WHITE);
    } else {
        DrawCircleV(pos, 15.0f, MAROON);
        DrawCircleLines(pos.x, pos.y, 15.0f, BLACK);
    }
    
    // Un icono simple o inicial
    if (enemy->name && enemy->name[0] != '\0') {
        char buf[2] = { enemy->name[0], '\0' };
        int w = MeasureText(buf, 14);
        DrawText(buf, pos.x - w/2, pos.y - 7, 14, WHITE);
    }

    // Indicador / insignia de estado de la IA (8.6: calma, alerta, persiguiendo)
    const char* badgeText = "-";
    Color badgeBg = (Color){ 70, 70, 90, 255 };
    switch (enemy->aiState) {
        case AIS_CHASING:
            badgeText = "!";
            badgeBg = (Color){ 220, 30, 30, 255 };
            break;
        case AIS_ALERT:
            badgeText = "?";
            badgeBg = (Color){ 230, 130, 20, 255 };
            break;
        case AIS_SEARCHING:
            badgeText = "?";
            badgeBg = (Color){ 210, 180, 30, 255 };
            break;
        case AIS_RETURNING:
            badgeText = "<";
            badgeBg = (Color){ 50, 140, 220, 255 };
            break;
        case AIS_PATROLLING:
            badgeText = "~";
            badgeBg = (Color){ 40, 180, 80, 255 };
            break;
        case AIS_RESTING:
            badgeText = "z";
            badgeBg = (Color){ 110, 110, 130, 255 };
            break;
        case AIS_IDLE:
        default:
            badgeText = "-";
            badgeBg = (Color){ 60, 60, 80, 255 };
            break;
    }
    DrawCircle(pos.x + 12, pos.y - 12, 8.0f, badgeBg);
    DrawCircleLines(pos.x + 12, pos.y - 12, 8.0f, RAYWHITE);
    DrawText(badgeText, pos.x + 9, pos.y - 17, 12, WHITE);

    // Tooltip al pasar el mouse por encima
    if (isHovered) {
        const char* bName = Enemy_GetBehaviorName(enemy->behavior);
        const char* sName = Enemy_GetAIStateName(enemy->aiState);
        char tip[128];
        snprintf(tip, sizeof(tip), "%s\n%s [%s]\nFza: %d | Mov: %d",
                 enemy->name, bName, sName, enemy->baseStrength, enemy->mobility);
        int tw = 175;
        int th = 55;
        DrawRectangle(pos.x + 18, pos.y - 35, tw, th, (Color){ 20, 20, 25, 235 });
        DrawRectangleLines(pos.x + 18, pos.y - 35, tw, th, (Color){ 160, 120, 60, 255 });
        DrawText(tip, pos.x + 23, pos.y - 30, 11, RAYWHITE);
    }
}
