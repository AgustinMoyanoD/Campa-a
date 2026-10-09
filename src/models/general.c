#include "models/general.h"
#include <string.h>

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
    memset(&g, 0, sizeof(General));

    if (name != NULL) {
        strncpy(g.name, name, sizeof(g.name) - 1);
        g.name[sizeof(g.name) - 1] = '\0';
    } else {
        strncpy(g.name, "General Anonimo", sizeof(g.name) - 1);
    }

    g.age = (age > 16) ? age : 16;
    g.vision = (vision >= 0) ? vision : 0;
    g.leadership = (leadership >= 0) ? leadership : 0;
    g.evaluation = (evaluation >= 0) ? evaluation : 0;
    g.planning = (planning >= 0) ? planning : 0;
    g.execution = (execution >= 0) ? execution : 0;

    g.cardCount = 0;
    g.curseCount = 0;
    return g;
}

Lieutenant Lieutenant_Create(
    const char* name,
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
    memset(&lt, 0, sizeof(Lieutenant));

    if (name != NULL) {
        strncpy(lt.name, name, sizeof(lt.name) - 1);
        lt.name[sizeof(lt.name) - 1] = '\0';
    } else {
        strncpy(lt.name, "Teniente Anonimo", sizeof(lt.name) - 1);
    }

    lt.age = (age > 16) ? age : 16;
    lt.vision = vision;
    lt.leadership = leadership;
    lt.evaluation = evaluation;
    lt.planning = planning;
    lt.execution = execution;

    if (loyalty < 0) loyalty = 0;
    if (loyalty > 100) loyalty = 100;
    lt.loyalty = loyalty;

    if (patronCardId && patronCardId[0] != '\0') {
        strncpy(lt.patronCard.id, patronCardId, 31);
        lt.patronCard.remaining = -1;
        lt.hasPatronCard = true;
    }
    lt.curseCount = 0;

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

int Commander_GetActiveModifierCount(CommanderRef cmd) {
    if (!cmd.ptr) return 0;
    if (cmd.type == OFFICER_GENERAL) {
        General* g = (General*)cmd.ptr;
        return g->cardCount + g->curseCount;
    } else {
        Lieutenant* lt = (Lieutenant*)cmd.ptr;
        return (lt->hasPatronCard ? 1 : 0) + lt->curseCount;
    }
}

const char* Commander_GetActiveModifier(CommanderRef cmd, int index) {
    if (!cmd.ptr || index < 0) return "";
    if (cmd.type == OFFICER_GENERAL) {
        General* g = (General*)cmd.ptr;
        if (index < g->cardCount) return g->cards[index].id;
        int curseIdx = index - g->cardCount;
        if (curseIdx < g->curseCount) return g->curses[curseIdx].id;
    } else {
        Lieutenant* lt = (Lieutenant*)cmd.ptr;
        int curseIdx = index;
        if (lt->hasPatronCard) {
            if (index == 0) return lt->patronCard.id;
            curseIdx = index - 1;
        }
        if (curseIdx < lt->curseCount) return lt->curses[curseIdx].id;
    }
    return "";
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
