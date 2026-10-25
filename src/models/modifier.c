#include "models/modifier.h"
#include "models/combat.h"
#include "controllers/game_master.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>

static void Modifier_TrimWhitespace(char* str) {
    if (!str) return;
    int len = strlen(str);
    while (len > 0 && (str[len-1] == ' ' || str[len-1] == '\r' || str[len-1] == '\n' || str[len-1] == '\t')) {
        str[len-1] = '\0';
        len--;
    }
    char* start = str;
    if ((unsigned char)start[0] == 0xEF && (unsigned char)start[1] == 0xBB && (unsigned char)start[2] == 0xBF) start += 3;
    while (*start == ' ' || *start == '\t') start++;
    if (start != str) {
        memmove(str, start, len - (start - str) + 1);
    }
}

static ModifierKind ParseModifierKind(const char* str) {
    if (strcmp(str, "CURSE") == 0) return MOD_KIND_CURSE;
    if (strcmp(str, "TRAIT") == 0) return MOD_KIND_TRAIT;
    if (strcmp(str, "EQUIPMENT") == 0) return MOD_KIND_EQUIPMENT;
    return MOD_KIND_CARD; // Default
}

static ModifierHook ParseHook(const char* str) {
    if (strcmp(str, "COMBAT_START") == 0) return HOOK_COMBAT_START;
    if (strcmp(str, "EVAL_ROLL") == 0) return HOOK_EVAL_ROLL;
    if (strcmp(str, "EVAL_REVEAL") == 0) return HOOK_EVAL_REVEAL;
    if (strcmp(str, "PLAN_ROLL") == 0) return HOOK_PLAN_ROLL;
    if (strcmp(str, "PLAN_OPTIONS") == 0) return HOOK_PLAN_OPTIONS;
    if (strcmp(str, "FLEE_COST") == 0) return HOOK_FLEE_COST;
    if (strcmp(str, "EXEC_ROLL") == 0) return HOOK_EXEC_ROLL;
    if (strcmp(str, "EXEC_DAMAGE") == 0) return HOOK_EXEC_DAMAGE;
    if (strcmp(str, "CASUALTIES") == 0) return HOOK_HEALTH_DAMAGE_RECEIVED;
    if (strcmp(str, "HEALTH_DAMAGE_RECEIVED") == 0) return HOOK_HEALTH_DAMAGE_RECEIVED;
    if (strcmp(str, "RECOUNT_DEATH") == 0) return HOOK_RECOUNT_DEATH;
    if (strcmp(str, "COMBAT_END") == 0) return HOOK_COMBAT_END;
    if (strcmp(str, "MAP_BASE_PA") == 0) return HOOK_MAP_BASE_PA;
    if (strcmp(str, "MAP_MOVE_COST") == 0) return HOOK_MAP_MOVE_COST;
    if (strcmp(str, "MAP_VISION") == 0) return HOOK_MAP_VISION;
    if (strcmp(str, "DAILY_CONSUMPTION") == 0) return HOOK_DAILY_CONSUMPTION;
    if (strcmp(str, "NIGHT_FALL") == 0) return HOOK_NIGHT_FALL;
    if (strcmp(str, "NIGHT_PENALTY") == 0) return HOOK_NIGHT_PENALTY;
    if (strcmp(str, "AMBUSH_ROLL") == 0) return HOOK_AMBUSH_ROLL;
    if (strcmp(str, "NIGHT_ENCOUNTER_CHANCE") == 0) return HOOK_NIGHT_ENCOUNTER_CHANCE;
    if (strcmp(str, "EVENT_FILTER") == 0) return HOOK_EVENT_FILTER;
    if (strcmp(str, "EVENT_RESULT") == 0) return HOOK_EVENT_RESULT;
    if (strcmp(str, "DAY_START") == 0) return HOOK_DAY_START;
    if (strcmp(str, "FORAGE_YIELD") == 0) return HOOK_FORAGE_YIELD;
    return HOOK_PASSIVE;
}

static ModifierTarget ParseTarget(const char* str, ModifierKind kind) {
    if (strcmp(str, "COMMANDER") == 0) return MOD_TARGET_COMMANDER;
    if (strcmp(str, "ARMY") == 0) return MOD_TARGET_ARMY;
    if (strcmp(str, "ENEMY") == 0) return MOD_TARGET_ENEMY;
    
    if (kind == MOD_KIND_CARD) return MOD_TARGET_COMMANDER;
    if (kind == MOD_KIND_CURSE) return MOD_TARGET_ARMY;
    if (kind == MOD_KIND_TRAIT) return MOD_TARGET_ENEMY;
    return MOD_TARGET_COMMANDER;
}

static Condition ParseCondition(const char* str) {
    Condition cond;
    memset(&cond, 0, sizeof(Condition));
    if (!str || str[0] == '\0') return cond;
    
    // Very basic parsing for Phase B, assuming 1 term for now
    char buffer[64];
    strncpy(buffer, str, sizeof(buffer)-1);
    char* op = strstr(buffer, "==");
    if (op) {
        *op = '\0';
        char* lhs = buffer;
        char* rhs = op + 2;
        Modifier_TrimWhitespace(lhs);
        Modifier_TrimWhitespace(rhs);
        if (strcmp(lhs, "plan") == 0) {
            cond.terms[0].lhs = COND_PLAN;
            cond.terms[0].op = OP_EQ;
            strncpy(cond.terms[0].rhsStr, rhs, sizeof(cond.terms[0].rhsStr)-1);
            cond.termCount = 1;
        } else if (strcasecmp(lhs, "time") == 0) {
            cond.terms[0].lhs = COND_TIME;
            cond.terms[0].op = OP_EQ;
            strncpy(cond.terms[0].rhsStr, rhs, sizeof(cond.terms[0].rhsStr)-1);
            cond.termCount = 1;
        } else if (strcmp(lhs, "origin") == 0) {
            cond.terms[0].lhs = COND_ORIGIN;
            cond.terms[0].op = OP_EQ;
            strncpy(cond.terms[0].rhsStr, rhs, sizeof(cond.terms[0].rhsStr)-1);
            cond.termCount = 1;
        } else if (strncmp(lhs, "enemy_tag(", 10) == 0) {
            cond.terms[0].lhs = COND_ENEMY_TAG;
            cond.terms[0].op = OP_EQ;
            // extract tag
            char* endP = strchr(lhs, ')');
            if (endP) {
                *endP = '\0';
                strncpy(cond.terms[0].arg, lhs+10, sizeof(cond.terms[0].arg)-1);
                cond.termCount = 1;
            }
        } else if (strncmp(lhs, "known_tag(", 10) == 0) {
            cond.terms[0].lhs = COND_KNOWN_TAG;
            cond.terms[0].op = OP_EQ;
            char* endP = strchr(lhs, ')');
            if (endP) {
                *endP = '\0';
                strncpy(cond.terms[0].arg, lhs+10, sizeof(cond.terms[0].arg)-1);
                cond.termCount = 1;
            }
        }
    } else {
        // support < > <= >= != as needed, or fail-closed
        // fail-closed for unrecognized
        printf("WARNING: Condicion invalida '%s'\n", str);
    }
    return cond;
}

static int LoadModifiersFromFile(const char* filepath, ModifierDef* outMods, int maxMods, int currentCount) {
    FILE* f = fopen(filepath, "r");
    if (!f) return currentCount;

    char line[256];
    ModifierDef* currentMod = NULL;
    ModifierRule* currentRule = NULL;
    
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        
        char buffer[256];
        strncpy(buffer, line, sizeof(buffer)-1);
        Modifier_TrimWhitespace(buffer);
        if (strlen(buffer) == 0) continue;
        
        if (strcmp(buffer, "[MODIFIER]") == 0) {
            if (currentMod && (int)currentMod->target == -1) {
                currentMod->target = ParseTarget("", currentMod->kind);
            }
            if (currentCount >= maxMods) break;
            currentMod = &outMods[currentCount];
            memset(currentMod, 0, sizeof(ModifierDef));
            currentMod->target = (ModifierTarget)-1;
            currentMod->durationType = DURATION_PERMANENT;
            currentMod->rarityWeight = 60; // COMMON by default
            currentMod->invocationClashMult = 1.0f;
            currentMod->invocationCounterIntent = -1;
            currentCount++;
            currentRule = NULL;
            continue;
        }
        else if (strcmp(buffer, "[RULE]") == 0 || strncmp(buffer, "[HOOK", 5) == 0) {
            if (currentMod && currentMod->ruleCount < MAX_RULES_PER_MODIFIER) {
                currentRule = &currentMod->rules[currentMod->ruleCount];
                currentMod->ruleCount++;
            }
            continue;
        }
        
        char* equals = strchr(buffer, '=');
        if (!equals) continue;
        
        *equals = '\0';
        char* key = buffer;
        char* val = equals + 1;
        Modifier_TrimWhitespace(key);
        Modifier_TrimWhitespace(val);
        
        if (currentRule != NULL) {
            if (strcmp(key, "Hook") == 0) {
                currentRule->hook = ParseHook(val);
            } else if (strcmp(key, "Condition") == 0) {
                currentRule->condition = ParseCondition(val);
            } else if (strcmp(key, "Effect") == 0) {
                if (currentRule->effectCount < MAX_EFFECTS_PER_RULE) {
                    if (Effect_ParseString(val, &currentRule->effects[currentRule->effectCount])) {
                        currentRule->effectCount++;
                    }
                }
            }
        }
        else if (currentMod != NULL) {
            if (strcmp(key, "Id") == 0) strncpy(currentMod->id, val, sizeof(currentMod->id)-1);
            else if (strcmp(key, "Name") == 0) strncpy(currentMod->name, val, sizeof(currentMod->name)-1);
            else if (strcmp(key, "Description") == 0) strncpy(currentMod->description, val, sizeof(currentMod->description)-1);
            else if (strcmp(key, "Kind") == 0) currentMod->kind = ParseModifierKind(val);
            else if (strcmp(key, "Target") == 0) currentMod->target = ParseTarget(val, currentMod->kind);
            else if (strcmp(key, "Source") == 0) strncpy(currentMod->source, val, sizeof(currentMod->source)-1);
            else if (strcmp(key, "Rarity") == 0) {
                if (strcmp(val, "COMMON") == 0) currentMod->rarityWeight = 60;
                else if (strcmp(val, "RARE") == 0) currentMod->rarityWeight = 30;
                else if (strcmp(val, "EPIC") == 0) currentMod->rarityWeight = 10;
                else if (strcmp(val, "LEGENDARY") == 0) currentMod->rarityWeight = 5;
            }
            else if (strcmp(key, "Tags") == 0) {
                char tagsBuf[128];
                strncpy(tagsBuf, val, sizeof(tagsBuf)-1);
                tagsBuf[sizeof(tagsBuf)-1] = '\0';
                char* token = strtok(tagsBuf, ",");
                while (token && currentMod->tagCount < 8) {
                    Modifier_TrimWhitespace(token);
                    if (strlen(token) > 0) {
                        strncpy(currentMod->tags[currentMod->tagCount++], token, 15);
                    }
                    token = strtok(NULL, ",");
                }
            }
            else if (strcmp(key, "Duration") == 0) {
                if (strncmp(val, "BATTLES:", 8) == 0) {
                    currentMod->durationType = DURATION_BATTLES;
                    currentMod->durationValue = atoi(val + 8);
                } else if (strncmp(val, "DAYS:", 5) == 0) {
                    currentMod->durationType = DURATION_DAYS;
                    currentMod->durationValue = atoi(val + 5);
                } else if (strcmp(val, "PERMANENT") == 0) {
                    currentMod->durationType = DURATION_PERMANENT;
                }
            }
            else if (strcmp(key, "Invocation_Name") == 0) {
                strncpy(currentMod->invocationName, val, sizeof(currentMod->invocationName)-1);
                currentMod->hasInvocation = true;
            }
            else if (strcmp(key, "Invocation_Desc") == 0) {
                strncpy(currentMod->invocationDesc, val, sizeof(currentMod->invocationDesc)-1);
                currentMod->hasInvocation = true;
            }
            else if (strcmp(key, "Invocation_Roll") == 0) {
                currentMod->invocationRollMod = atoi(val);
                currentMod->hasInvocation = true;
            }
            else if (strcmp(key, "Invocation_Clash") == 0) {
                currentMod->invocationClashMult = (float)atof(val);
                currentMod->hasInvocation = true;
            }
            else if (strcmp(key, "Invocation_Counter") == 0) {
                if (strcmp(val, "CHARGE") == 0) currentMod->invocationCounterIntent = 0;
                else if (strcmp(val, "HOLD") == 0) currentMod->invocationCounterIntent = 1;
                else if (strcmp(val, "MANEUVER") == 0) currentMod->invocationCounterIntent = 2;
                else currentMod->invocationCounterIntent = atoi(val);
                currentMod->hasInvocation = true;
            }
            else if (strcmp(key, "Invocation_Variance") == 0) {
                currentMod->invocationHighVariance = (strcmp(val, "HIGH") == 0 || strcmp(val, "TRUE") == 0 || strcmp(val, "1") == 0);
                currentMod->hasInvocation = true;
            }
            else if (strcmp(key, "Rival") == 0) {
                strncpy(currentMod->rivalGod, val, sizeof(currentMod->rivalGod)-1);
            }
        }
    }
    
    // Post process Target if not specified
    if (currentMod && (int)currentMod->target == -1) {
        currentMod->target = ParseTarget("", currentMod->kind);
    }
    
    fclose(f);
    return currentCount;
}

int Modifier_LoadAll(const char* directory, ModifierDef* outMods, int maxMods) {
    int count = 0;
    DIR* dir = opendir(directory);
    if (!dir) return 0;
    struct dirent* ent;
    while ((ent = readdir(dir)) != NULL && count < maxMods) {
        if (strstr(ent->d_name, ".mod")) {
            char path[512];
            snprintf(path, sizeof(path), "%s/%s", directory, ent->d_name);
            count = LoadModifiersFromFile(path, outMods, maxMods, count);
        }
    }
    closedir(dir);
    return count;
}

ModifierDef* Modifier_GetById(const ModifierDef* registry, int registryCount, const char* id) {
    if (!registry || !id) return NULL;
    for (int i=0; i<registryCount; i++) {
        if (strcmp(registry[i].id, id) == 0) {
            return (ModifierDef*)&registry[i];
        }
    }
    return NULL;
}

int Modifier_CollectActive(const ModifierRegistry* reg, const Army* army, const EnemySquad* enemy, ActiveMod* out, int max) {
    if (!reg || !army || !out || max <= 0) return 0;
    int count = 0;
    
    // 1. Commander Divine Cards (ignorar si están agotadas)
    int cardCount = Commander_GetCardCount(army->commander);
    for (int i = 0; i < cardCount && count < max; i++) {
        if (Commander_IsCardExhausted(army->commander, i)) {
            continue; // La carta agotada no aporta pasivas
        }
        const char* cardId = Commander_GetCard(army->commander, i);
        ModifierDef* def = Modifier_GetById(reg->defs, reg->count, cardId);
        if (def) {
            out[count].def = def;
            out[count].owner = SIDE_PLAYER;
            count++;
        }
    }
    
    // 2. Commander Curses
    int cmdCurseCount = Commander_GetCurseCount(army->commander);
    for (int i = 0; i < cmdCurseCount && count < max; i++) {
        const char* curseId = Commander_GetCurse(army->commander, i);
        ModifierDef* def = Modifier_GetById(reg->defs, reg->count, curseId);
        if (def) {
            out[count].def = def;
            out[count].owner = SIDE_PLAYER;
            count++;
        }
    }
    
    // 3. Army Curses
    for (int i = 0; i < army->curseCount && count < max; i++) {
        ModifierDef* def = Modifier_GetById(reg->defs, reg->count, army->curses[i].id);
        if (def) {
            out[count].def = def;
            out[count].owner = SIDE_PLAYER;
            count++;
        }
    }
    
    return count;
}

static bool Condition_Eval(const Condition* cond, const ModContext* ctx) {
    if (!cond || cond->termCount == 0) return true;
    for (int i = 0; i < cond->termCount; i++) {
        const CondTerm* term = &cond->terms[i];
        bool passed = false;
        
        if (term->lhs == COND_PLAN) {
            if (ctx->combat) {
                const char* currentPlanStr = "NONE";
                if (ctx->combat->selectedPlan == PLAN_ASSAULT) currentPlanStr = "ASSAULT";
                else if (ctx->combat->selectedPlan == PLAN_SKIRMISH) currentPlanStr = "SKIRMISH";
                else if (ctx->combat->selectedPlan == PLAN_FLEE) currentPlanStr = "FLEE";
                
                if (term->op == OP_EQ && strcmp(term->rhsStr, currentPlanStr) == 0) passed = true;
                if (term->op == OP_NE && strcmp(term->rhsStr, currentPlanStr) != 0) passed = true;
            }
        } else if (term->lhs == COND_TIME) {
            if (strcasecmp(term->rhsStr, "NIGHT") == 0) {
                passed = (ctx->timeOfDay == 2);
            } else if (strcasecmp(term->rhsStr, "AFTERNOON") == 0) {
                passed = (ctx->timeOfDay == 1);
            } else if (strcasecmp(term->rhsStr, "MORNING") == 0) {
                passed = (ctx->timeOfDay == 0);
            }
        } else if (term->lhs == COND_ENEMY_TAG) {
            // Simplified for now
            passed = false;
            // TODO: check enemy tags via Enemy_IsTagKnown
        } else if (term->lhs == COND_ORIGIN) {
            // Check origin of modifier... wait, condition applies to target but evaluates context. We don't have source string here.
            // Ignore for now.
            passed = true;
        }
        
        if (!passed) return false;
    }
    return true;
}

bool Modifier_CheckRivals(const ModifierRegistry* reg, const Army* army, char* outConflict, int maxLen) {
    if (!reg || !army) return false;
    int cardCount = Commander_GetCardCount(army->commander);
    if (cardCount < 2) return false;

    for (int i = 0; i < cardCount; i++) {
        const char* idA = Commander_GetCard(army->commander, i);
        const ModifierDef* defA = Modifier_GetById(reg->defs, reg->count, idA);
        if (!defA || defA->rivalGod[0] == '\0') continue;

        for (int j = 0; j < cardCount; j++) {
            if (i == j) continue;
            const char* idB = Commander_GetCard(army->commander, j);
            const ModifierDef* defB = Modifier_GetById(reg->defs, reg->count, idB);
            if (!defB) continue;

            if (strcasecmp(defA->rivalGod, defB->source) == 0 ||
                (defB->rivalGod[0] != '\0' && strcasecmp(defB->rivalGod, defA->source) == 0)) {
                if (outConflict && maxLen > 0) {
                    snprintf(outConflict, maxLen, "Disputa: %s vs %s", defA->source, defB->source);
                }
                return true;
            }
        }
    }
    return false;
}

int Modifier_Query(ModifierHook hook, EffectType stat, ModSide side, const ModContext* ctx, int base, ModBreakdown* outOpt) {
    if (!ctx || !ctx->reg || !ctx->army) return base;
    
    if (outOpt) {
        outOpt->base = base;
        outOpt->count = 0;
        outOpt->final = base;
    }
    
    int absoluteBonus = 0;
    int percentageBonus = 0;
    
    ActiveMod activeMods[32];
    int count = Modifier_CollectActive(ctx->reg, ctx->army, ctx->enemy, activeMods, 32);
    
    for (int i = 0; i < count; i++) {
        const ModifierDef* mod = activeMods[i].def;
        if (!mod) continue;
        
        for (int r = 0; r < mod->ruleCount; r++) {
            const ModifierRule* rule = &mod->rules[r];
            if (rule->hook != hook) continue;
            
            if (!Condition_Eval(&rule->condition, ctx)) continue;
            
            for (int e = 0; e < rule->effectCount; e++) {
                const EffectDef* effect = &rule->effects[e];
                if (effect->type != stat) continue;
                if (effect->side != side) continue;
                
                if (effect->mode == EFFECT_MODE_ABSOLUTE) {
                    absoluteBonus += effect->value;
                    if (outOpt && outOpt->count < 12) {
                        strncpy(outOpt->entries[outOpt->count].source, mod->name, 47);
                        outOpt->entries[outOpt->count].flat = effect->value;
                        outOpt->entries[outOpt->count].percent = 0;
                        outOpt->count++;
                    }
                } else if (effect->mode == EFFECT_MODE_PERCENTAGE) {
                    percentageBonus += effect->value;
                    if (outOpt && outOpt->count < 12) {
                        strncpy(outOpt->entries[outOpt->count].source, mod->name, 47);
                        outOpt->entries[outOpt->count].flat = 0;
                        outOpt->entries[outOpt->count].percent = effect->value;
                        outOpt->count++;
                    }
                }
            }
        }
    }
    
    int finalValue = base + absoluteBonus;
    if (percentageBonus != 0) {
        finalValue += (finalValue * percentageBonus) / 100;
    }

    // Penalización por rivalidad divina si los dioses disputan en el campamento (-1 a planificación)
    if (hook == HOOK_PLAN_ROLL && stat == EFFECT_PLANNING && side == EFFECT_SIDE_SELF) {
        char rivalMsg[64];
        if (Modifier_CheckRivals(ctx->reg, ctx->army, rivalMsg, sizeof(rivalMsg))) {
            finalValue -= 1;
            if (outOpt && outOpt->count < 12) {
                strncpy(outOpt->entries[outOpt->count].source, rivalMsg, 47);
                outOpt->entries[outOpt->count].flat = -1;
                outOpt->entries[outOpt->count].percent = 0;
                outOpt->count++;
            }
        }
    }
    
    if (outOpt) outOpt->final = finalValue;
    return finalValue;
}


int Modifier_CollectActions(ModifierHook hook, const ModContext* ctx, ModAction* out, int max) {
    if (!ctx || !ctx->reg || !ctx->army || !out || max <= 0) return 0;
    
    int numActions = 0;
    ActiveMod activeMods[32];
    int count = Modifier_CollectActive(ctx->reg, ctx->army, ctx->enemy, activeMods, 32);
    
    for (int i = 0; i < count; i++) {
        const ModifierDef* mod = activeMods[i].def;
        if (!mod) continue;
        
        for (int r = 0; r < mod->ruleCount; r++) {
            const ModifierRule* rule = &mod->rules[r];
            if (rule->hook != hook) continue;
            
            if (!Condition_Eval(&rule->condition, ctx)) continue;
            
            for (int e = 0; e < rule->effectCount; e++) {
                const EffectDef* effect = &rule->effects[e];
                // Actions typically don't have EFFECT_MODE_ABSOLUTE on stats, they are things like EFFECT_REVEAL_TAG, EFFECT_ADDMOD, etc.
                if (numActions < max) {
                    out[numActions].type = effect->type;
                    strncpy(out[numActions].arg, effect->stringValue, 31);
                    out[numActions].value = effect->value;
                    out[numActions].source = mod;
                    numActions++;
                }
            }
        }
    }
    return numActions;
}



