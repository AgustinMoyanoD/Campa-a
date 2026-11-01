#include "game/controllers/movement_controller.h"
#include "game/controllers/npc_controller.h"
#include "game/presentation/audio_manager.h"
#include "engine/presentation/tween.h"
#include "engine/presentation/sequence.h"
#include "engine/presentation/screen_fx.h"
#include "engine/core/hex_grid.h"

void Movement_UpdateStep(GameMaster* gm, Map* map, Army* army, MapViewState* mapView)
{
    if (!gm || !map || !army || !mapView) return;
    if (!army->isMoving || gm->phase == PHASE_EVENT || Sequence_IsBusy()) return;

    if (army->currentPathIndex < army->pathLength) {
        HexCoord nextPos = army->movePath[army->currentPathIndex];
        /* Búsqueda O(1) indexada en vez de bucle O(N) (Hallazgo 3) */
        HexTile* targetTile = Map_GetTile(map, nextPos);

        if (targetTile && GameMaster_AttemptMove(gm, map, army, targetTile)) {
            Map_UpdateVision(map, army->position,
                             Army_GetVision(army) + GameMaster_GetVisionModifier(gm, army));
            army->currentPathIndex++;

            Vector2 targetPixel = HexToPixel(army->position, mapView->hexRadius);

            SeqAction moveAct = {0};
            moveAct.duration = 0.25f;
            Sequence_Push(moveAct);

            Tween_Start(&mapView->visualArmyPos.x, targetPixel.x, 0.25f, Ease_Linear);
            Tween_Start(&mapView->visualArmyPos.y, targetPixel.y, 0.25f, Ease_Linear);
            Audio_PlayStepBiome(targetTile->biome);
            ScreenFX_SpawnMarchDust((Vector2){ targetPixel.x + mapView->origin.x, targetPixel.y + mapView->origin.y },
                                    (Color){ 195, 175, 130, 255 });

            for (int e = 0; e < gm->enemyCount; e++) {
                if (gm->enemies[e].isActive &&
                    gm->enemies[e].position.q == army->position.q &&
                    gm->enemies[e].position.r == army->position.r)
                {
                    if (NPC_IsHostileToPlayer(&gm->enemies[e], gm)) {
                        Audio_PlaySfxByName("enemy_alert");
                        bool ambushed = (gm->enemies[e].behavior == AI_AMBUSH || Enemy_HasTag(&gm->enemies[e], TAG_AMBUSH));
                        NPC_InitiateCombat(gm, army, &gm->enemies[e], 0, ambushed);
                        army->isMoving = false;
                        break;
                    }
                }
            }
        } else {
            army->isMoving = false;
        }
    } else {
        army->isMoving = false;
    }
}

void Movement_HandleCombatRetreat(GameMaster* gm, const Map* map, const Army* army, MapViewState* mapView, CameraController* cc)
{
    (void)map;
    if (!gm || !army || !mapView) return;

    Vector2 retreatTarget = HexToPixel(army->position, mapView->hexRadius);
    if (gm->activeCombat.endReason == COMBAT_END_DEFEAT || gm->activeCombat.endReason == COMBAT_END_FLEE) {
        SeqAction retAct = {0};
        retAct.duration = 0.45f;
        Sequence_Push(retAct);
        Tween_Start(&mapView->visualArmyPos.x, retreatTarget.x, 0.45f, Ease_OutCubic);
        Tween_Start(&mapView->visualArmyPos.y, retreatTarget.y, 0.45f, Ease_OutCubic);
    } else {
        mapView->visualArmyPos = retreatTarget;
        if (cc) cc->camera.target = mapView->visualArmyPos;
    }
}
