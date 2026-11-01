#include "views/npc_view.h"
#include "views/map_view.h"
#include "controllers/npc_controller.h"
#include "controllers/game_master.h"
#include "presentation/ui_widgets.h"
#include "presentation/ui_theme.h"
#include <stdio.h>
#include <math.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

/* =========================================================================
   CONSULTAS AUXILIARES DE COLOR Y TEXTO DIPLOMÁTICO
   ========================================================================= */

Color NPCView_GetRelationColor(FactionOpinionLevel relation) {
    switch (relation) {
        case FACTION_HOSTILE:
            return (Color){ 220, 40, 40, 255 };    /* Rojo brillante / carmesí */
        case FACTION_SUSPICIOUS:
            return (Color){ 220, 140, 30, 255 };   /* Ámbar / naranja */
        case FACTION_NEUTRAL:
            return (Color){ 180, 190, 205, 255 };  /* Gris azulado / blanco hueso */
        case FACTION_FRIENDLY:
            return (Color){ 50, 185, 120, 255 };   /* Verde oliva / turquesa */
        case FACTION_ALLIED:
            return (Color){ 230, 200, 60, 255 };   /* Dorado / esmeralda */
        default:
            return (Color){ 180, 190, 205, 255 };
    }
}

const char* NPCView_GetFactionName(int factionId, const struct GameMaster* gm) {
    if (factionId < 0) return "Independiente / Forajidos";
    if (gm && factionId >= 0 && factionId < 3) {
        return gm->factions[factionId].name;
    }
    switch (factionId) {
        case 0: return "Gremio de Comerciantes";
        case 1: return "Legion del Sol";
        case 2: return "Hermandad del Hierro";
        default: return "Desconocida";
    }
}

/* =========================================================================
   RENDERIZADO DE HERÁLDICA Y EMBLEMAS DE FACCIÓN
   ========================================================================= */

static void DrawMiniStar(Vector2 center, float rOuter, float rInner, Color col) {
    Vector2 pts[10];
    for (int i = 0; i < 10; i++) {
        float angle = -PI/2.0f + (float)i * (PI / 5.0f);
        float r = (i % 2 == 0) ? rOuter : rInner;
        pts[i] = (Vector2){ center.x + cosf(angle) * r, center.y + sinf(angle) * r };
    }
    for (int i = 0; i < 10; i++) {
        int next = (i + 1) % 10;
        DrawTriangle(center, pts[i], pts[next], col);
    }
}

static void DrawFactionEmblem(Vector2 pos, int factionId) {
    switch (factionId) {
        case -1: {
            /* Bandidos / Forajidos: Daga Estilizada de Asalto */
            // Empuñadura y pomo
            DrawCircle((int)pos.x, (int)(pos.y - 6.5f), 1.6f, (Color){ 210, 215, 225, 255 });
            DrawLineEx((Vector2){ pos.x, pos.y - 6.5f }, (Vector2){ pos.x, pos.y - 3.0f }, 1.5f, (Color){ 175, 180, 190, 255 });
            // Guarda cruzada
            DrawLineEx((Vector2){ pos.x - 5.5f, pos.y - 3.0f }, (Vector2){ pos.x + 5.5f, pos.y - 3.0f }, 1.8f, (Color){ 220, 180, 80, 255 });
            // Hoja afilada
            Vector2 p1 = { pos.x - 2.8f, pos.y - 2.0f };
            Vector2 p2 = { pos.x + 2.8f, pos.y - 2.0f };
            Vector2 p3 = { pos.x, pos.y + 7.5f };
            DrawTriangle(p1, p3, p2, (Color){ 245, 248, 255, 255 });
            // Filo central
            DrawLineEx((Vector2){ pos.x, pos.y - 2.0f }, (Vector2){ pos.x, pos.y + 6.0f }, 1.0f, (Color){ 130, 140, 155, 255 });
            break;
        }
        case 0: {
            /* Gremio de Comerciantes: Balanza de Comercio */
            Color gold = (Color){ 245, 210, 70, 255 };
            // Mástil vertical
            DrawLineEx((Vector2){ pos.x, pos.y - 6.5f }, (Vector2){ pos.x, pos.y + 6.0f }, 1.5f, gold);
            DrawCircle((int)pos.x, (int)(pos.y - 6.5f), 1.8f, (Color){ 255, 235, 140, 255 });
            // Brazo horizontal
            DrawLineEx((Vector2){ pos.x - 6.5f, pos.y - 3.5f }, (Vector2){ pos.x + 6.5f, pos.y - 3.5f }, 1.5f, gold);
            // Platillo izquierdo
            DrawLineEx((Vector2){ pos.x - 6.5f, pos.y - 3.5f }, (Vector2){ pos.x - 8.5f, pos.y + 1.5f }, 1.0f, (Color){ 210, 180, 80, 220 });
            DrawLineEx((Vector2){ pos.x - 6.5f, pos.y - 3.5f }, (Vector2){ pos.x - 4.5f, pos.y + 1.5f }, 1.0f, (Color){ 210, 180, 80, 220 });
            DrawLineEx((Vector2){ pos.x - 9.0f, pos.y + 1.5f }, (Vector2){ pos.x - 4.0f, pos.y + 1.5f }, 1.5f, gold);
            // Platillo derecho
            DrawLineEx((Vector2){ pos.x + 6.5f, pos.y - 3.5f }, (Vector2){ pos.x + 4.5f, pos.y + 1.5f }, 1.0f, (Color){ 210, 180, 80, 220 });
            DrawLineEx((Vector2){ pos.x + 6.5f, pos.y - 3.5f }, (Vector2){ pos.x + 8.5f, pos.y + 1.5f }, 1.0f, (Color){ 210, 180, 80, 220 });
            DrawLineEx((Vector2){ pos.x + 4.0f, pos.y + 1.5f }, (Vector2){ pos.x + 9.0f, pos.y + 1.5f }, 1.5f, gold);
            // Moneda en la base
            DrawCircle((int)pos.x, (int)(pos.y + 5.0f), 1.6f, (Color){ 255, 235, 120, 255 });
            break;
        }
        case 1: {
            /* Legión del Sol: Disco Solar Radiante */
            Color sunCore = (Color){ 255, 225, 75, 255 };
            Color sunRays = (Color){ 255, 175, 40, 255 };
            // Núcleo solar
            DrawCircle((int)pos.x, (int)pos.y, 4.0f, sunCore);
            DrawCircleLines((int)pos.x, (int)pos.y, 4.2f, sunRays);
            // 4 Rayos cardinales
            DrawLineEx((Vector2){ pos.x, pos.y - 4.0f }, (Vector2){ pos.x, pos.y - 7.5f }, 1.5f, sunRays);
            DrawLineEx((Vector2){ pos.x, pos.y + 4.0f }, (Vector2){ pos.x, pos.y + 7.5f }, 1.5f, sunRays);
            DrawLineEx((Vector2){ pos.x - 4.0f, pos.y }, (Vector2){ pos.x - 7.5f, pos.y }, 1.5f, sunRays);
            DrawLineEx((Vector2){ pos.x + 4.0f, pos.y }, (Vector2){ pos.x + 7.5f, pos.y }, 1.5f, sunRays);
            // 4 Rayos diagonales
            DrawLineEx((Vector2){ pos.x - 2.8f, pos.y - 2.8f }, (Vector2){ pos.x - 5.5f, pos.y - 5.5f }, 1.2f, sunCore);
            DrawLineEx((Vector2){ pos.x + 2.8f, pos.y - 2.8f }, (Vector2){ pos.x + 5.5f, pos.y - 5.5f }, 1.2f, sunCore);
            DrawLineEx((Vector2){ pos.x - 2.8f, pos.y + 2.8f }, (Vector2){ pos.x - 5.5f, pos.y + 5.5f }, 1.2f, sunCore);
            DrawLineEx((Vector2){ pos.x + 2.8f, pos.y + 2.8f }, (Vector2){ pos.x + 5.5f, pos.y + 5.5f }, 1.2f, sunCore);
            break;
        }
        case 2: {
            /* Hermandad del Hierro: Yunque Forjado */
            Color steelLight = (Color){ 225, 235, 245, 255 };
            Color steelMid   = (Color){ 165, 175, 190, 255 };
            Color steelBase  = (Color){ 135, 145, 160, 255 };
            // Tabla plana superior
            DrawRectangle((int)pos.x - 6, (int)pos.y - 3, 12, 3, steelLight);
            // Cuerno cónico izquierdo
            Vector2 t1 = { pos.x - 6.0f, pos.y - 3.0f };
            Vector2 t2 = { pos.x - 6.0f, pos.y };
            Vector2 t3 = { pos.x - 8.8f, pos.y - 1.5f };
            DrawTriangle(t1, t2, t3, steelLight);
            // Cuerpo central
            DrawRectangle((int)pos.x - 3, (int)pos.y, 6, 3, steelMid);
            // Base ancha pesada
            DrawRectangle((int)pos.x - 6, (int)pos.y + 3, 12, 3, steelBase);
            DrawLine((int)pos.x - 6, (int)pos.y + 3, (int)pos.x + 6, (int)pos.y + 3, (Color){ 200, 210, 225, 255 });
            break;
        }
        default: {
            // Símbolo genérico / Escudo
            DrawCircle((int)pos.x, (int)pos.y, 4.0f, (Color){ 200, 200, 210, 255 });
            break;
        }
    }
}

/* =========================================================================
   FUNCIÓN PRINCIPAL: NPCView_DrawOnMap
   ========================================================================= */

void NPCView_DrawOnMap(const NPCSquad* npc, const struct GameMaster* gm, 
                       Vector2 visualPos, Vector2 origin, 
                       const GameAssets* assets, bool isHovered, float hexRadius) {
    if (!npc || !npc->isActive) return;

    Vector2 pos = visualPos;
    pos.x += origin.x;
    pos.y += origin.y - 10.0f; // Ajuste vertical basico para perspectiva isometrica

    // 1. Estado Diplomático
    FactionOpinionLevel relation = NPC_GetRelationWithPlayer(npc, gm);
    bool isHostile = NPC_IsHostileToPlayer(npc, gm);
    Color relationCol = NPCView_GetRelationColor(relation);

    // 2. Rango de Percepción / Amenaza (Al hacer Hover con el ratón)
    if (isHovered) {
        int sense = (npc->senseRadius > 0) ? npc->senseRadius : 3;
        float threatWorldRadius = (float)sense * hexRadius * 1.5f;

        if (isHostile && npc->aiState != AIS_RESTING) {
            // Amenaza Hostil Activa (peligro letal de intercepción)
            DrawCircleLines(pos.x, pos.y, threatWorldRadius, Fade((Color){ 220, 40, 40, 255 }, 0.75f));
            DrawCircle(pos.x, pos.y, threatWorldRadius, Fade((Color){ 200, 30, 30, 255 }, 0.12f));
        } else {
            // Percepción Neutral / Amistosa / Aliada o Enemigo Descansando/Saqueando (no hostil temporalmente)
            Color ringCol = (npc->aiState == AIS_RESTING) ? (Color){ 170, 155, 120, 255 } : relationCol;
            DrawCircleLines(pos.x, pos.y, threatWorldRadius, Fade(ringCol, 0.45f));
            DrawCircle(pos.x, pos.y, threatWorldRadius, Fade(ringCol, 0.05f));
        }
    }

    // 3. Sombra proyectada del Token
    DrawCircle((int)pos.x + 2, (int)pos.y + 3, 16.0f, (Color){ 10, 12, 16, 175 });

    // 4. Fondo temático según facción
    Color baseBg = (Color){ 28, 30, 38, 255 };
    switch (npc->factionId) {
        case -1: baseBg = (Color){ 48, 18, 24, 255 }; break;  /* Carmesí oscuro forajido */
        case 0:  baseBg = (Color){ 18, 34, 48, 255 }; break;  /* Marino / pizarra gremio */
        case 1:  baseBg = (Color){ 54, 24, 28, 255 }; break;  /* Púrpura solar imperial */
        case 2:  baseBg = (Color){ 32, 36, 42, 255 }; break;  /* Acero templado hierro */
    }
    DrawCircleV(pos, 15.0f, baseBg);

    // 5. Destello especial si es Caravana de Tesoro / Caudales (lootMultiplier > 1.0f)
    if (npc->lootMultiplier > 1.0f) {
        float pulse = (sinf((float)GetTime() * 6.0f) + 1.0f) * 0.5f;
        Color glowCol = Fade(UI_COLOR_GOLD_LIGHT, 0.45f + 0.45f * pulse);
        DrawCircleLines((int)pos.x, (int)pos.y, 17.5f + pulse * 2.0f, glowCol);
        DrawCircleLines((int)pos.x, (int)pos.y, 18.5f + pulse * 2.0f, Fade(UI_COLOR_GOLD, 0.35f * pulse));
    }

    // 6. Anillo / Borde del Token según Relación Diplomática (Sección 3.A)
    DrawCircleLines((int)pos.x, (int)pos.y, 15.0f, relationCol);
    DrawCircleLines((int)pos.x, (int)pos.y, 16.0f, relationCol);
    DrawCircleLines((int)pos.x, (int)pos.y, 13.5f, Fade(relationCol, 0.4f));

    // 7. Heráldica / Emblema de la Facción (Sección 3.C)
    DrawFactionEmblem(pos, npc->factionId);

    // 8. Insignia especial de Botín Doble / Caudales (arriba a la izquierda si lootMultiplier > 1.0f)
    if (npc->lootMultiplier > 1.0f) {
        Vector2 lpos = { pos.x - 13.0f, pos.y - 13.0f };
        DrawCircle((int)lpos.x, (int)lpos.y, 7.5f, (Color){ 45, 35, 12, 255 });
        DrawCircleLines((int)lpos.x, (int)lpos.y, 7.5f, UI_COLOR_GOLD_LIGHT);
        // Estrella dorada de 5 puntas
        DrawMiniStar(lpos, 4.2f, 1.8f, UI_COLOR_GOLD_LIGHT);
    }

    // 9. Insignia de Estado de la IA (arriba a la derecha - Sección 6)
    Vector2 bPos = { pos.x + 13.0f, pos.y - 13.0f };
    Color badgeBg = (Color){ 65, 70, 82, 255 };
    switch (npc->aiState) {
        case AIS_CHASING:    badgeBg = (Color){ 175, 28, 28, 255 };  break; /* Espadas rojas */
        case AIS_ALERT:      badgeBg = (Color){ 165, 110, 18, 255 }; break; /* Ojo amarillo */
        case AIS_SEARCHING:  badgeBg = (Color){ 145, 115, 28, 255 }; break; /* Lupa / Huella */
        case AIS_RETURNING:  badgeBg = (Color){ 38, 110, 175, 255 }; break; /* Flecha retorno */
        case AIS_PATROLLING: badgeBg = (Color){ 36, 135, 72, 255 };  break; /* Flechas circulares */
        case AIS_RESTING:    badgeBg = (Color){ 92, 74, 58, 255 };   break; /* Tienda / Fogata */
        case AIS_IDLE:
        default:             badgeBg = (Color){ 65, 70, 82, 255 };   break; /* Escudo gris */
    }
    DrawCircle((int)bPos.x, (int)bPos.y, 9.0f, badgeBg);
    DrawCircleLines((int)bPos.x, (int)bPos.y, 9.0f, RAYWHITE);

    switch (npc->aiState) {
        case AIS_IDLE: {
            /* Escudo gris */
            DrawRectangle((int)(bPos.x - 3.5f), (int)(bPos.y - 4.0f), 7, 5, (Color){ 195, 202, 215, 255 });
            DrawTriangle((Vector2){ bPos.x - 3.5f, bPos.y + 1.0f },
                         (Vector2){ bPos.x, bPos.y + 5.0f },
                         (Vector2){ bPos.x + 3.5f, bPos.y + 1.0f },
                         (Color){ 195, 202, 215, 255 });
            break;
        }
        case AIS_PATROLLING: {
            /* Flechas circulares */
            DrawCircleLines((int)bPos.x, (int)bPos.y, 4.2f, WHITE);
            DrawTriangle((Vector2){ bPos.x + 2.5f, bPos.y - 5.5f },
                         (Vector2){ bPos.x + 5.8f, bPos.y - 2.0f },
                         (Vector2){ bPos.x + 1.8f, bPos.y - 2.0f }, WHITE);
            break;
        }
        case AIS_ALERT: {
            /* Ojo amarillo */
            Color eyeYel = (Color){ 255, 235, 90, 255 };
            DrawLineEx((Vector2){ bPos.x - 5.0f, bPos.y }, (Vector2){ bPos.x, bPos.y - 3.2f }, 1.5f, eyeYel);
            DrawLineEx((Vector2){ bPos.x, bPos.y - 3.2f }, (Vector2){ bPos.x + 5.0f, bPos.y }, 1.5f, eyeYel);
            DrawLineEx((Vector2){ bPos.x - 5.0f, bPos.y }, (Vector2){ bPos.x, bPos.y + 3.2f }, 1.5f, eyeYel);
            DrawLineEx((Vector2){ bPos.x, bPos.y + 3.2f }, (Vector2){ bPos.x + 5.0f, bPos.y }, 1.5f, eyeYel);
            DrawCircle((int)bPos.x, (int)bPos.y, 1.8f, eyeYel);
            break;
        }
        case AIS_CHASING: {
            /* Espadas rojas cruzadas */
            Color swCol = (Color){ 255, 225, 220, 255 };
            DrawLineEx((Vector2){ bPos.x - 4.2f, bPos.y + 4.2f }, (Vector2){ bPos.x + 4.2f, bPos.y - 4.2f }, 1.8f, swCol);
            DrawLineEx((Vector2){ bPos.x - 4.2f, bPos.y - 4.2f }, (Vector2){ bPos.x + 4.2f, bPos.y + 4.2f }, 1.8f, swCol);
            break;
        }
        case AIS_SEARCHING: {
            /* Lupa / Huella */
            DrawCircleLines((int)(bPos.x - 1.2f), (int)(bPos.y - 1.2f), 3.2f, (Color){ 255, 240, 150, 255 });
            DrawLineEx((Vector2){ bPos.x + 1.2f, bPos.y + 1.2f }, (Vector2){ bPos.x + 4.5f, bPos.y + 4.5f }, 1.8f, (Color){ 255, 240, 150, 255 });
            break;
        }
        case AIS_RETURNING: {
            /* Flecha retorno */
            DrawLineEx((Vector2){ bPos.x + 4.2f, bPos.y }, (Vector2){ bPos.x - 3.0f, bPos.y }, 1.8f, WHITE);
            DrawTriangle((Vector2){ bPos.x - 5.0f, bPos.y },
                         (Vector2){ bPos.x - 1.0f, bPos.y + 3.5f },
                         (Vector2){ bPos.x - 1.0f, bPos.y - 3.5f }, WHITE);
            break;
        }
        case AIS_RESTING: {
            /* Tienda / Fogata */
            DrawTriangle((Vector2){ bPos.x, bPos.y - 4.8f },
                         (Vector2){ bPos.x - 4.8f, bPos.y + 3.5f },
                         (Vector2){ bPos.x + 4.8f, bPos.y + 3.5f },
                         (Color){ 235, 205, 155, 255 });
            DrawCircle((int)bPos.x, (int)(bPos.y + 2.2f), 2.0f, (Color){ 255, 130, 40, 255 });
            break;
        }
        default: break;
    }

    // 10. Tooltip Enriquecido al Pasar el Ratón (Sección 3.D)
    if (isHovered) {
        const char* fName = NPCView_GetFactionName(npc->factionId, gm);
        const char* rLevelName = Faction_GetLevelName(relation);
        const char* bName = Enemy_GetBehaviorName(npc->behavior);
        const char* sName = Enemy_GetAIStateName(npc->aiState);

        Color opColor = UI_COLOR_BAD;
        switch (relation) {
            case FACTION_HOSTILE:    opColor = (Color){ 235, 60, 60, 255 }; break;
            case FACTION_SUSPICIOUS: opColor = (Color){ 235, 150, 40, 255 }; break;
            case FACTION_NEUTRAL:    opColor = (Color){ 200, 210, 225, 255 }; break;
            case FACTION_FRIENDLY:   opColor = (Color){ 70, 205, 130, 255 }; break;
            case FACTION_ALLIED:     opColor = (Color){ 245, 215, 65, 255 }; break;
        }

        bool isTreasure = (npc->lootMultiplier > 1.0f);
        float tw = 290.0f;
        float th = isTreasure ? 134.0f : 112.0f;

        int sw = GetScreenWidth();
        int sh = GetScreenHeight();

        float tx = pos.x + 22.0f;
        float ty = pos.y - 40.0f;
        if (tx + tw > (float)(sw - 12)) tx = pos.x - tw - 20.0f;
        if (tx < 12.0f) tx = 12.0f;
        if (ty + th > (float)(sh - 12)) ty = (float)(sh - 12) - th;
        if (ty < 12.0f) ty = 12.0f;

        Rectangle tipRect = { tx, ty, tw, th };

        // Fondo y marco 9-slice del tooltip
        DrawRectangle((int)tx + 3, (int)ty + 4, (int)tw, (int)th, (Color){ 6, 8, 12, 165 });
        UI_DrawPanelStyled(tipRect, UI_STYLE_PANEL_TOOLTIP);

        // Línea 1: Nombre del Escuadrón (Color según Facción/Relación)
        float curY = ty + 7.0f;
        UI_DrawText(npc->name ? npc->name : "Escuadron", (Vector2){ tx + 10.0f, curY }, 14, relationCol);
        curY += 18.0f;

        // Línea 2: Facción y Opinión (Prefijo neutro + nivel con color reactivo)
        char facPrefix[64];
        snprintf(facPrefix, sizeof(facPrefix), "Faccion: %s ", fName);
        Vector2 prefSz = UI_MeasureText(facPrefix, 12);
        UI_DrawText(facPrefix, (Vector2){ tx + 10.0f, curY }, 12, (Color){ 205, 210, 225, 255 });
        char opBuf[32];
        snprintf(opBuf, sizeof(opBuf), "[%s]", rLevelName);
        UI_DrawText(opBuf, (Vector2){ tx + 10.0f + prefSz.x, curY }, 12, opColor);
        curY += 17.0f;

        // Separador sutil
        DrawLine((int)(tx + 8.0f), (int)curY, (int)(tx + tw - 8.0f), (int)curY, (Color){ 90, 75, 60, 200 });
        curY += 5.0f;

        // Línea 3: Estado de la IA y Comportamiento
        char stateBuf[96];
        if (npc->aiState == AIS_RESTING) {
            snprintf(stateBuf, sizeof(stateBuf), "Estado: %s (No hostil)", sName);
        } else {
            snprintf(stateBuf, sizeof(stateBuf), "Estado: %s [%s]", sName, bName);
        }
        UI_DrawText(stateBuf, (Vector2){ tx + 10.0f, curY }, 12, (npc->aiState == AIS_RESTING) ? (Color){ 245, 205, 135, 255 } : RAYWHITE);
        curY += 16.0f;

        // Línea 4: Fuerza y Movilidad
        char statsBuf[96];
        snprintf(statsBuf, sizeof(statsBuf), "Fuerza: %d | Mov: %d PA", npc->baseStrength, npc->mobility);
        UI_DrawText(statsBuf, (Vector2){ tx + 10.0f, curY }, 12, (Color){ 205, 210, 225, 255 });
        curY += 16.0f;

        // Línea 5: Composición de Tropas
        char troopsBuf[96];
        snprintf(troopsBuf, sizeof(troopsBuf), "Tropas: %d Ini / %d Sol / %d Sgt", npc->initiates, npc->soldiers, npc->sergeants);
        UI_DrawText(troopsBuf, (Vector2){ tx + 10.0f, curY }, 12, (Color){ 175, 180, 195, 255 });
        curY += 16.0f;

        // Línea 6: Cargamento de Caudales (solo si lootMultiplier > 1.0f)
        if (isTreasure) {
            char lootBuf[96];
            if (npc->lootMultiplier == 2.0f) {
                snprintf(lootBuf, sizeof(lootBuf), "[*] Cargamento de Caudales: Botin Doble x2");
            } else {
                snprintf(lootBuf, sizeof(lootBuf), "[*] Cargamento Especial: Botin x%.1f", npc->lootMultiplier);
            }
            UI_DrawText(lootBuf, (Vector2){ tx + 10.0f, curY }, 12, UI_COLOR_GOLD_LIGHT);
        }
    }
}
