#include "models/general.h"
#include <string.h>

static const GeneralConfig s_generalPresets[] = {
    {
        .name = "Cayo Marcio",
        .age = 38,
        .stats = { .vision = 5, .leadership = 5, .evaluation = 5, .planning = 5, .execution = 5 },
        .initialCardCount = 0
    },
    {
        .name = "Aulo Postumio",
        .age = 46,
        .stats = { .vision = 4, .leadership = 6, .evaluation = 6, .planning = 6, .execution = 3 },
        .initialCardCount = 0
    },
    {
        .name = "Quinto Sertorio",
        .age = 31,
        .stats = { .vision = 6, .leadership = 5, .evaluation = 4, .planning = 4, .execution = 6 },
        .initialCardCount = 0
    }
};

static const LieutenantConfig s_lieutenantPresets[] = {
    {
        .name = "Publio Varro",
        .age = 26,
        .role = ROLE_EXPLORADOR,
        .patronGod = "Hermes",
        .patronCardId = "hermes_pie_alado",
        .stats = { .vision = 2, .leadership = 1, .evaluation = 1, .planning = 0, .execution = 2 },
        .loyalty = 70
    },
    {
        .name = "Marco Craso",
        .age = 34,
        .role = ROLE_INTENDENTE,
        .patronGod = "Deméter",
        .patronCardId = "demeter_cosecha",
        .stats = { .vision = 1, .leadership = 2, .evaluation = 2, .planning = 1, .execution = 1 },
        .loyalty = 65
    },
    {
        .name = "Lucio Vitelio",
        .age = 42,
        .role = ROLE_CAPELLAN,
        .patronGod = "Hestia",
        .patronCardId = "hestia_hogar",
        .stats = { .vision = 1, .leadership = 1, .evaluation = 2, .planning = 2, .execution = 0 },
        .loyalty = 75
    }
};

int General_GetPresetCount(void) {
    return (int)(sizeof(s_generalPresets) / sizeof(s_generalPresets[0]));
}

GeneralConfig General_GetPreset(int index) {
    int count = General_GetPresetCount();
    if (index < 0 || index >= count) index = 0;
    return s_generalPresets[index];
}

int Lieutenant_GetPresetCount(void) {
    return (int)(sizeof(s_lieutenantPresets) / sizeof(s_lieutenantPresets[0]));
}

LieutenantConfig Lieutenant_GetPreset(int index) {
    int count = Lieutenant_GetPresetCount();
    if (index < 0 || index >= count) index = 0;
    return s_lieutenantPresets[index];
}

void General_Init(General* general, const GeneralConfig* cfg) {
    if (!general) return;
    memset(general, 0, sizeof(General));
    if (!cfg) return;

    if (cfg->name != NULL) {
        strncpy(general->name, cfg->name, sizeof(general->name) - 1);
        general->name[sizeof(general->name) - 1] = '\0';
    } else {
        strncpy(general->name, "General Anonimo", sizeof(general->name) - 1);
    }

    general->age = (cfg->age > 16) ? cfg->age : 16;
    general->vision = (cfg->stats.vision >= 0) ? cfg->stats.vision : 0;
    general->leadership = (cfg->stats.leadership >= 0) ? cfg->stats.leadership : 0;
    general->evaluation = (cfg->stats.evaluation >= 0) ? cfg->stats.evaluation : 0;
    general->planning = (cfg->stats.planning >= 0) ? cfg->stats.planning : 0;
    general->execution = (cfg->stats.execution >= 0) ? cfg->stats.execution : 0;

    general->cardCount = 0;
    general->curseCount = 0;

    for (int i = 0; i < cfg->initialCardCount && i < MAX_GENERAL_DIVINE_CARDS; i++) {
        if (cfg->initialCards[i] && cfg->initialCards[i][0] != '\0') {
            strncpy(general->cards[general->cardCount].id, cfg->initialCards[i], 31);
            general->cards[general->cardCount].remaining = -1;
            general->cardCount++;
        }
    }
}

General General_Create(
    const char* name,
    int age,
    int vision,
    int leadership,
    int evaluation,
    int planning,
    int execution
)
{
    General g;
    GeneralConfig cfg = {
        .name = name,
        .age = age,
        .stats = {
            .vision = vision,
            .leadership = leadership,
            .evaluation = evaluation,
            .planning = planning,
            .execution = execution
        },
        .initialCardCount = 0
    };
    General_Init(&g, &cfg);
    return g;
}

const char* OfficerRole_GetName(OfficerRole role) {
    switch (role) {
        case ROLE_EXPLORADOR: return "Explorador";
        case ROLE_INTENDENTE: return "Intendente";
        case ROLE_CAPELLAN:   return "Capellan";
        default:              return "Oficial";
    }
}

void Lieutenant_Init(Lieutenant* lt, const LieutenantConfig* cfg) {
    if (!lt) return;
    memset(lt, 0, sizeof(Lieutenant));
    if (!cfg) return;

    if (cfg->name != NULL) {
        strncpy(lt->name, cfg->name, sizeof(lt->name) - 1);
        lt->name[sizeof(lt->name) - 1] = '\0';
    } else {
        strncpy(lt->name, "Teniente Anonimo", sizeof(lt->name) - 1);
    }

    lt->role = cfg->role;
    if (cfg->patronGod != NULL) {
        strncpy(lt->patronGod, cfg->patronGod, sizeof(lt->patronGod) - 1);
        lt->patronGod[sizeof(lt->patronGod) - 1] = '\0';
    } else {
        strncpy(lt->patronGod, "Desconocido", sizeof(lt->patronGod) - 1);
    }

    lt->age = (cfg->age > 16) ? cfg->age : 16;
    lt->vision = cfg->stats.vision;
    lt->leadership = cfg->stats.leadership;
    lt->evaluation = cfg->stats.evaluation;
    lt->planning = cfg->stats.planning;
    lt->execution = cfg->stats.execution;

    int loyalty = cfg->loyalty;
    if (loyalty < 0) loyalty = 0;
    if (loyalty > 100) loyalty = 100;
    lt->loyalty = loyalty;

    if (cfg->patronCardId && cfg->patronCardId[0] != '\0') {
        strncpy(lt->patronCard.id, cfg->patronCardId, 31);
        lt->patronCard.remaining = -1;
        lt->hasPatronCard = true;
    }
    lt->curseCount = 0;
}

Lieutenant Lieutenant_Create(
    const char* name,
    OfficerRole role,
    const char* patronGod,
    const char* patronCardId,
    int age,
    int vision,
    int leadership,
    int evaluation,
    int planning,
    int execution,
    int loyalty
)
{
    Lieutenant lt;
    LieutenantConfig cfg = {
        .name = name,
        .age = age,
        .role = role,
        .patronGod = patronGod,
        .patronCardId = patronCardId,
        .stats = {
            .vision = vision,
            .leadership = leadership,
            .evaluation = evaluation,
            .planning = planning,
            .execution = execution
        },
        .loyalty = loyalty
    };
    Lieutenant_Init(&lt, &cfg);
    return lt;
}

CommanderRef Commander_FromGeneral(General* general) {
    CommanderRef ref;
    ref.type = OFFICER_GENERAL;
    ref.ptr = (void*)general;
    return ref;
}

CommanderRef Commander_FromLieutenant(Lieutenant* lt) {
    CommanderRef ref;
    ref.type = OFFICER_LIEUTENANT;
    ref.ptr = (void*)lt;
    return ref;
}

bool Commander_IsValid(CommanderRef cmd) {
    return (cmd.ptr != NULL);
}

const char* Commander_GetName(CommanderRef cmd) {
    if (cmd.ptr == NULL) return "Sin Comandante";
    if (cmd.type == OFFICER_GENERAL) return ((General*)cmd.ptr)->name;
    else return ((Lieutenant*)cmd.ptr)->name;
}

int Commander_GetAge(CommanderRef cmd) {
    if (cmd.ptr == NULL) return 0;
    if (cmd.type == OFFICER_GENERAL) return ((General*)cmd.ptr)->age;
    else return ((Lieutenant*)cmd.ptr)->age;
}

int Commander_GetVision(CommanderRef cmd) {
    if (cmd.ptr == NULL) return 0;
    if (cmd.type == OFFICER_GENERAL) return ((General*)cmd.ptr)->vision;
    else return ((Lieutenant*)cmd.ptr)->vision;
}

int Commander_GetLeadership(CommanderRef cmd) {
    if (cmd.ptr == NULL) return 0;
    if (cmd.type == OFFICER_GENERAL) return ((General*)cmd.ptr)->leadership;
    else return ((Lieutenant*)cmd.ptr)->leadership;
}

int Commander_GetEvaluation(CommanderRef cmd) {
    if (cmd.ptr == NULL) return 0;
    if (cmd.type == OFFICER_GENERAL) return ((General*)cmd.ptr)->evaluation;
    else return ((Lieutenant*)cmd.ptr)->evaluation;
}

int Commander_GetPlanning(CommanderRef cmd) {
    if (cmd.ptr == NULL) return 0;
    if (cmd.type == OFFICER_GENERAL) return ((General*)cmd.ptr)->planning;
    else return ((Lieutenant*)cmd.ptr)->planning;
}

int Commander_GetExecution(CommanderRef cmd) {
    if (cmd.ptr == NULL) return 0;
    if (cmd.type == OFFICER_GENERAL) return ((General*)cmd.ptr)->execution;
    else return ((Lieutenant*)cmd.ptr)->execution;
}

int Commander_GetLoyalty(CommanderRef cmd) {
    if (cmd.ptr == NULL) return 0;
    if (cmd.type == OFFICER_GENERAL) return 100;
    else return ((Lieutenant*)cmd.ptr)->loyalty;
}

int Commander_GetCardCount(CommanderRef cmd) {
    if (!cmd.ptr) return 0;
    if (cmd.type == OFFICER_GENERAL) {
        return ((General*)cmd.ptr)->cardCount;
    } else {
        return ((Lieutenant*)cmd.ptr)->hasPatronCard ? 1 : 0;
    }
}

const char* Commander_GetCard(CommanderRef cmd, int index) {
    if (!cmd.ptr || index < 0) return "";
    if (cmd.type == OFFICER_GENERAL) {
        General* g = (General*)cmd.ptr;
        if (index < g->cardCount) return g->cards[index].id;
    } else {
        Lieutenant* lt = (Lieutenant*)cmd.ptr;
        if (index == 0 && lt->hasPatronCard) return lt->patronCard.id;
    }
    return "";
}

int Commander_GetCurseCount(CommanderRef cmd) {
    if (!cmd.ptr) return 0;
    if (cmd.type == OFFICER_GENERAL) {
        return ((General*)cmd.ptr)->curseCount;
    } else {
        return ((Lieutenant*)cmd.ptr)->curseCount;
    }
}

const char* Commander_GetCurse(CommanderRef cmd, int index) {
    if (!cmd.ptr || index < 0) return "";
    if (cmd.type == OFFICER_GENERAL) {
        General* g = (General*)cmd.ptr;
        if (index < g->curseCount) return g->curses[index].id;
    } else {
        Lieutenant* lt = (Lieutenant*)cmd.ptr;
        if (index < lt->curseCount) return lt->curses[index].id;
    }
    return "";
}

int Commander_GetActiveModifierCount(CommanderRef cmd) {
    return Commander_GetCardCount(cmd);
}

const char* Commander_GetActiveModifier(CommanderRef cmd, int index) {
    return Commander_GetCard(cmd, index);
}

bool Commander_AddActiveModifier(CommanderRef cmd, const char* modId) {
    // Legacy function, using it to add cards if possible, otherwise curses
    if (!cmd.ptr || !modId) return false;
    if (cmd.type == OFFICER_GENERAL) {
        General* g = (General*)cmd.ptr;
        if (g->cardCount < 3) {
            strncpy(g->cards[g->cardCount].id, modId, 31);
            g->cards[g->cardCount].remaining = -1;
            g->cardCount++;
            return true;
        } else if (g->curseCount < 4) {
            strncpy(g->curses[g->curseCount].id, modId, 31);
            g->curses[g->curseCount].remaining = -1;
            g->curseCount++;
            return true;
        }
    } else {
        Lieutenant* lt = (Lieutenant*)cmd.ptr;
        if (!lt->hasPatronCard) {
            strncpy(lt->patronCard.id, modId, 31);
            lt->patronCard.remaining = -1;
            lt->hasPatronCard = true;
            return true;
        } else if (lt->curseCount < 4) {
            strncpy(lt->curses[lt->curseCount].id, modId, 31);
            lt->curses[lt->curseCount].remaining = -1;
            lt->curseCount++;
            return true;
        }
    }
    return false;
}

ModifierInstance* Commander_GetCardInstance(CommanderRef cmd, int index) {
    if (!cmd.ptr || index < 0) return NULL;
    if (cmd.type == OFFICER_GENERAL) {
        General* g = (General*)cmd.ptr;
        if (index < g->cardCount) return &g->cards[index];
    } else {
        Lieutenant* lt = (Lieutenant*)cmd.ptr;
        if (index == 0 && lt->hasPatronCard) return &lt->patronCard;
    }
    return NULL;
}

bool Commander_IsCardExhausted(CommanderRef cmd, int index) {
    ModifierInstance* inst = Commander_GetCardInstance(cmd, index);
    return inst ? inst->isExhausted : false;
}

void Commander_SetCardExhausted(CommanderRef cmd, int index, bool exhausted) {
    ModifierInstance* inst = Commander_GetCardInstance(cmd, index);
    if (inst) {
        inst->isExhausted = exhausted;
        if (exhausted) {
            inst->battlesUntilRecharge = 2; // 2 batallas o descanso
        } else {
            inst->battlesUntilRecharge = 0;
        }
    }
}
