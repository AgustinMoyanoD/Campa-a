#include "general.h"
#include <string.h>

/* ============================================================
   CREACIÓN DE GENERAL
   ============================================================ */

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

    if (name != NULL)
    {
        strncpy(g.name, name, sizeof(g.name) - 1);
        g.name[sizeof(g.name) - 1] = '\0';
    }
    else
    {
        strncpy(g.name, "General Anonimo", sizeof(g.name) - 1);
    }

    g.age = (age > 16) ? age : 16;

    /* El General nunca puede tener estadísticas negativas (mínimo 0) */
    g.vision = (vision >= 0) ? vision : 0;
    g.leadership = (leadership >= 0) ? leadership : 0;
    g.evaluation = (evaluation >= 0) ? evaluation : 0;
    g.planning = (planning >= 0) ? planning : 0;
    g.execution = (execution >= 0) ? execution : 0;

    g.divineCardCount = 0;
    for (int i = 0; i < MAX_GENERAL_DIVINE_CARDS; i++)
    {
        g.divineCards[i] = 0;
    }

    return g;
}


/* ============================================================
   CREACIÓN DE TENIENTE
   ============================================================ */

Lieutenant Lieutenant_Create(
    const char* name,
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

    if (name != NULL)
    {
        strncpy(lt.name, name, sizeof(lt.name) - 1);
        lt.name[sizeof(lt.name) - 1] = '\0';
    }
    else
    {
        strncpy(lt.name, "Teniente Anonimo", sizeof(lt.name) - 1);
    }

    lt.age = (age > 16) ? age : 16;

    /* Los tenientes pueden tener valores reducidos o incluso negativos */
    lt.vision = vision;
    lt.leadership = leadership;
    lt.evaluation = evaluation;
    lt.planning = planning;
    lt.execution = execution;

    /* Lealtad limitada de 0 a 100 */
    if (loyalty < 0) loyalty = 0;
    if (loyalty > 100) loyalty = 100;
    lt.loyalty = loyalty;

    lt.divineCard = 0;
    lt.hasDivineCard = false;

    return lt;
}


/* ============================================================
   INTERFAZ POLIMÓRFICA DE COMANDANTE (GENERAL O TENIENTE)
   ============================================================ */

CommanderRef Commander_FromGeneral(General* general)
{
    CommanderRef ref;
    ref.type = OFFICER_GENERAL;
    ref.ptr = (void*)general;
    return ref;
}

CommanderRef Commander_FromLieutenant(Lieutenant* lt)
{
    CommanderRef ref;
    ref.type = OFFICER_LIEUTENANT;
    ref.ptr = (void*)lt;
    return ref;
}

bool Commander_IsValid(CommanderRef cmd)
{
    return (cmd.ptr != NULL);
}

const char* Commander_GetName(CommanderRef cmd)
{
    if (cmd.ptr == NULL) return "Sin Comandante";

    if (cmd.type == OFFICER_GENERAL)
        return ((General*)cmd.ptr)->name;
    else
        return ((Lieutenant*)cmd.ptr)->name;
}

int Commander_GetAge(CommanderRef cmd)
{
    if (cmd.ptr == NULL) return 0;

    if (cmd.type == OFFICER_GENERAL)
        return ((General*)cmd.ptr)->age;
    else
        return ((Lieutenant*)cmd.ptr)->age;
}

int Commander_GetVision(CommanderRef cmd)
{
    if (cmd.ptr == NULL) return 0;

    if (cmd.type == OFFICER_GENERAL)
        return ((General*)cmd.ptr)->vision;
    else
        return ((Lieutenant*)cmd.ptr)->vision;
}

int Commander_GetLeadership(CommanderRef cmd)
{
    if (cmd.ptr == NULL) return 0;

    if (cmd.type == OFFICER_GENERAL)
        return ((General*)cmd.ptr)->leadership;
    else
        return ((Lieutenant*)cmd.ptr)->leadership;
}

int Commander_GetEvaluation(CommanderRef cmd)
{
    if (cmd.ptr == NULL) return 0;

    if (cmd.type == OFFICER_GENERAL)
        return ((General*)cmd.ptr)->evaluation;
    else
        return ((Lieutenant*)cmd.ptr)->evaluation;
}

int Commander_GetPlanning(CommanderRef cmd)
{
    if (cmd.ptr == NULL) return 0;

    if (cmd.type == OFFICER_GENERAL)
        return ((General*)cmd.ptr)->planning;
    else
        return ((Lieutenant*)cmd.ptr)->planning;
}

int Commander_GetExecution(CommanderRef cmd)
{
    if (cmd.ptr == NULL) return 0;

    if (cmd.type == OFFICER_GENERAL)
        return ((General*)cmd.ptr)->execution;
    else
        return ((Lieutenant*)cmd.ptr)->execution;
}

int Commander_GetLoyalty(CommanderRef cmd)
{
    if (cmd.ptr == NULL) return 0;

    if (cmd.type == OFFICER_GENERAL)
        return 100; /* El General tiene 100% de lealtad a sí mismo */
    else
        return ((Lieutenant*)cmd.ptr)->loyalty;
}

int Commander_GetDivineCardCount(CommanderRef cmd)
{
    if (cmd.ptr == NULL) return 0;

    if (cmd.type == OFFICER_GENERAL)
    {
        return ((General*)cmd.ptr)->divineCardCount;
    }
    else
    {
        return ((Lieutenant*)cmd.ptr)->hasDivineCard ? 1 : 0;
    }
}

int Commander_GetDivineCard(CommanderRef cmd, int index)
{
    if (cmd.ptr == NULL) return 0;

    if (cmd.type == OFFICER_GENERAL)
    {
        General* g = (General*)cmd.ptr;
        if (index >= 0 && index < g->divineCardCount)
            return g->divineCards[index];
    }
    else
    {
        Lieutenant* lt = (Lieutenant*)cmd.ptr;
        if (index == 0 && lt->hasDivineCard)
            return lt->divineCard;
    }

    return 0;
}

bool Commander_AddDivineCard(CommanderRef cmd, int cardId)
{
    if (cmd.ptr == NULL || cardId <= 0) return false;

    if (cmd.type == OFFICER_GENERAL)
    {
        General* g = (General*)cmd.ptr;
        if (g->divineCardCount < MAX_GENERAL_DIVINE_CARDS)
        {
            g->divineCards[g->divineCardCount++] = cardId;
            return true;
        }
    }
    else
    {
        Lieutenant* lt = (Lieutenant*)cmd.ptr;
        if (!lt->hasDivineCard)
        {
            lt->divineCard = cardId;
            lt->hasDivineCard = true;
            return true;
        }
    }

    return false;
}

bool Commander_ReplaceDivineCard(CommanderRef cmd, int slotIndex, int cardId)
{
    if (cmd.ptr == NULL || cardId <= 0) return false;

    if (cmd.type == OFFICER_GENERAL)
    {
        General* g = (General*)cmd.ptr;
        if (slotIndex >= 0 && slotIndex < g->divineCardCount)
        {
            g->divineCards[slotIndex] = cardId;
            return true;
        }
    }
    else
    {
        Lieutenant* lt = (Lieutenant*)cmd.ptr;
        if (slotIndex == 0)
        {
            lt->divineCard = cardId;
            lt->hasDivineCard = true;
            return true;
        }
    }

    return false;
}
