#include "game/controllers/event_flow.h"
#include "game/controllers/npc_controller.h"
#include "game/presentation/audio_manager.h"
#include "game/views/event_view.h"
#include <string.h>

void EventFlow_Init(EventFlowState* state)
{
    if (!state) return;
    memset(state, 0, sizeof(EventFlowState));
}

void EventFlow_Update(EventFlowState* state, GameMaster* gm, Army* playerArmy, Map* map, float dt)
{
    if (!state || !gm || !playerArmy || !map) return;
    if (gm->phase != PHASE_EVENT || (!gm->activeEvent && !gm->activeResult)) return;

    gm->eventTimer += dt;

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    /* Feedback sonoro al abrir nuevo evento sin variables static (Hallazgo 4) */
    if (gm->activeEvent != state->lastAudioEvent && gm->activeEvent != NULL) {
        Audio_PlaySfxByName("event_open");
        state->lastAudioEvent = gm->activeEvent;
    } else if (gm->activeEvent == NULL && gm->activeResult == NULL) {
        state->lastAudioEvent = NULL;
    }

    if (gm->activeResult) {
        int r_opt = (gm->eventTimer > 1.0f) ? ResultView_HandleInput(gm->activeResult, sw, sh) : -1;
        if (r_opt == 0) {
            Event_ApplyResult(gm->activeResult, playerArmy, gm);
            for (int ei = 0; ei < gm->activeResult->effectCount; ei++) {
                EffectDef* eff = &gm->activeResult->effects[ei];
                if (eff->type == EFFECT_ADD_MOD && strlen(eff->stringValue) > 0) {
                    ModifierDef* def = Modifier_GetById(gm->modRegistry.defs, gm->modRegistry.count, eff->stringValue);
                    if (def) {
                        Army_EquipModifier(playerArmy, def);
                        if (def->kind == MOD_KIND_CURSE) Audio_PlaySfxByName("curse_acquired");
                    }
                } else if (eff->type == EFFECT_REMOVE_MOD && strlen(eff->stringValue) > 0) {
                    Army_RemoveModifier(playerArmy, eff->stringValue);
                }
            }

            if (gm->activeResult->startsBattle) {
                gm->pendingCityAfterEvent = false;
                bool ambushed = false;
                int initiator = 0;
                state->eventEnemy = NPC_CreateEventSquad(gm->activeResult->battleEnemy, gm->activeResult->id, &ambushed, &initiator);
                NPC_InitiateCombat(gm, playerArmy, &state->eventEnemy, initiator, ambushed);
            } else {
                if (strcmp(gm->activeResult->id, "res_general_regresa_si") == 0) {
                    Army_RestoreGeneral(playerArmy);
                } else if (strcmp(gm->activeResult->id, "res_general_regresa_no") == 0) {
                    Army_DismissGeneral(playerArmy);
                }

                if (gm->pendingCityAfterEvent) {
                    gm->pendingCityAfterEvent = false;
                    HexTile* cTile = Map_GetTile(map, gm->pendingCityCoord);
                    int fId = cTile ? cTile->structure.factionId : -1;
                    bool isHostile = false;
                    if (fId >= 0 && fId < 3) {
                        const BalanceConfig* bcfg = Balance_GetGlobal();
                        isHostile = (Faction_GetLevel(gm->factions[fId].opinion, bcfg) == FACTION_HOSTILE);
                    }
                    if (gm->pendingCityBlocked || isHostile ||
                        GameMaster_HasFlag(gm, "bloqueo_ciudad") ||
                        GameMaster_HasFlag(gm, "puertas_cerradas"))
                    {
                        gm->phase = PHASE_PLAYER;
                    } else {
                        GameMaster_OpenCity(gm, map, playerArmy);
                    }
                } else {
                    gm->phase = PHASE_PLAYER;
                }
            }
            gm->activeResult = NULL;
        }
    } else if (gm->activeEvent) {
        Event_EvaluateOptions(gm->activeEvent, playerArmy, gm);

        int opt = (gm->eventTimer > 1.0f) ? EventView_HandleInput(gm->activeEvent, sw, sh) : -1;
        if (opt >= 0 && opt < gm->activeEvent->optionCount) {
            EventOption* chosenOpt = &gm->activeEvent->options[opt];
            if (chosenOpt->resultPoolCount > 0) {
                int totalWeight = 0;
                for (int w = 0; w < chosenOpt->resultPoolCount; w++) {
                    totalWeight += (chosenOpt->resultWeights[w] > 0) ? chosenOpt->resultWeights[w] : 1;
                }

                /* Sorteo determinista con Rng (Hallazgo 2) */
                int roll = (totalWeight > 1) ? Rng_Range(&gm->rng, 1, totalWeight) : 1;
                int accum = 0;
                int chosenIdx = 0;
                for (int w = 0; w < chosenOpt->resultPoolCount; w++) {
                    accum += (chosenOpt->resultWeights[w] > 0) ? chosenOpt->resultWeights[w] : 1;
                    if (roll <= accum) {
                        chosenIdx = w;
                        break;
                    }
                }

                const char* resId = chosenOpt->resultPool[chosenIdx];
                EventResult* foundRes = GameMaster_FindResult(gm, resId);
                if (foundRes) {
                    gm->activeResult = foundRes;
                    gm->activeEvent = NULL;
                    gm->eventTimer = 0.0f;
                } else {
                    gm->phase = PHASE_PLAYER;
                    gm->activeEvent = NULL;
                }
            } else {
                gm->phase = PHASE_PLAYER;
                gm->activeEvent = NULL;
            }
        }
    }
}

void EventFlow_Draw(const GameMaster* gm, int sw, int sh)
{
    if (!gm || gm->phase != PHASE_EVENT) return;

    if (gm->activeResult) {
        ResultView_Draw(gm->activeResult, sw, sh);
    } else if (gm->activeEvent) {
        EventView_Draw(gm->activeEvent, sw, sh);
    }
}
