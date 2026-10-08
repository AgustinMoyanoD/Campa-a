#include "army.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>

/* ============================================================
   CREACIÓN DE LA ARMY
   ============================================================ */

Army Army_Create(
    HexCoord startPos,
    General general,
    int sargentos,
    int soldados,
    int iniciados,
    float agua,
    float comida,
    int monedas,
    int ganado
)
{
    Army army;
    memset(&army, 0, sizeof(Army));

    army.position = startPos;
    army.general = general;
    army.lieutenantCount = 0;

    /* Por defecto, el General está al mando inicial de la Army */
    army.commander = Commander_FromGeneral(&army.general);

    army.sargentos = (sargentos >= 0) ? sargentos : 0;
    army.soldados = (soldados >= 0) ? soldados : 0;
    army.iniciados = (iniciados >= 0) ? iniciados : 0;

    army.agua_disponible = (agua >= 0.0f) ? agua : 0.0f;
    army.comida_disponible = (comida >= 0.0f) ? comida : 0.0f;
    army.monedas = (monedas >= 0) ? monedas : 0;
    army.ganado = (ganado >= 0) ? ganado : 0;

    army.moral = 80.0f;  /* Moral inicial sólida */
    army.salud = 100.0f; /* Salud plena */
    army.infamia = 0;

    return army;
}

int Army_GetTotalSoldiers(const Army* army)
{
    if (army == NULL) return 0;
    return army->sargentos + army->soldados + army->iniciados;
}


/* ============================================================
   GESTIÓN DE OFICIALES
   ============================================================ */

bool Army_AddLieutenant(Army* army, Lieutenant lt)
{
    if (army == NULL || army->lieutenantCount >= MAX_ARMY_LIEUTENANTS)
        return false;

    army->lieutenants[army->lieutenantCount] = lt;
    army->lieutenantCount++;
    return true;
}

void Army_SetCommanderGeneral(Army* army)
{
    if (army == NULL) return;
    army->commander = Commander_FromGeneral(&army->general);
}

bool Army_SetCommanderLieutenant(Army* army, int lieutenantIndex)
{
    if (army == NULL || lieutenantIndex < 0 || lieutenantIndex >= army->lieutenantCount)
        return false;

    army->commander = Commander_FromLieutenant(&army->lieutenants[lieutenantIndex]);
    return true;
}


/* ============================================================
   CONSUMO LOGÍSTICO DE RECURSOS
   ============================================================ */

float Army_CalculateWaterUsage(int cantSoldados, int cantGanado)
{
    /* Cada soldado consume 1.0 unidad de agua.
       Cada animal vivo consume 1.5 unidades de agua. */
    return ((float)cantSoldados * 1.0f) + ((float)cantGanado * 1.5f);
}

float Army_CalculateFoodUsage(int cantSoldados, int cantGanado)
{
    /* Cada soldado consume 1.0 ración de comida.
       El ganado consume 0.2 raciones adicionales de mantenimiento. */
    return ((float)cantSoldados * 1.0f) + ((float)cantGanado * 0.2f);
}

void Army_ConsumeTurnResources(Army* army)
{
    if (army == NULL) return;

    int totalSoldiers = Army_GetTotalSoldiers(army);
    float waterNeeded = Army_CalculateWaterUsage(totalSoldiers, army->ganado);
    float foodNeeded = Army_CalculateFoodUsage(totalSoldiers, army->ganado);

    /* Consumo de agua */
    if (army->agua_disponible >= waterNeeded)
    {
        army->agua_disponible -= waterNeeded;
    }
    else
    {
        /* Escasez de agua: deshidratación y caída de moral */
        army->agua_disponible = 0.0f;
        army->moral -= 8.0f;
        army->salud -= 4.0f;
    }

    /* Consumo de comida */
    if (army->comida_disponible >= foodNeeded)
    {
        army->comida_disponible -= foodNeeded;
    }
    else
    {
        /* Hambruna: se puede sacrificar ganado automáticamente o mermar salud */
        if (army->ganado > 0)
        {
            /* Sacrificio de emergencia: 1 cabeza de ganado aporta 15 de comida */
            army->ganado--;
            army->comida_disponible += 15.0f;
            if (army->comida_disponible >= foodNeeded)
            {
                army->comida_disponible -= foodNeeded;
            }
            else
            {
                army->comida_disponible = 0.0f;
                army->moral -= 5.0f;
            }
        }
        else
        {
            army->comida_disponible = 0.0f;
            army->moral -= 12.0f;
            army->salud -= 8.0f;
        }
    }

    /* Clamping de indicadores */
    if (army->moral < 0.0f) army->moral = 0.0f;
    if (army->moral > 100.0f) army->moral = 100.0f;
    if (army->salud < 0.0f) army->salud = 0.0f;
    if (army->salud > 100.0f) army->salud = 100.0f;
}


/* ============================================================
   FÓRMULAS DE ESTADÍSTICAS GLOBALES DE LA ARMY
   ============================================================ */

/*
    Fórmula de Ejecución:
    f_ejecución = Ejecución_comandante + sargentos + (soldados / B) + f(iniciados)

    Para los iniciados:
    - Tener pocos iniciados aporta una bonificación positiva (refuerzos frescos).
    - Conforme el número de iniciados crece demasiado sin suficiente disciplina,
      el término decae y se convierte en una penalización neta (caos, estorbo, desorden).
*/
float Army_CalculateExecution(const Army* army, float B)
{
    if (army == NULL) return 0.0f;

    if (B <= 0.05f) B = DEFAULT_EXECUTION_B;

    float execCommander = (float)Commander_GetExecution(army->commander);
    float valSargentos = (float)army->sargentos * 1.5f; /* Sargentos aportan liderazgo táctico */
    float valSoldados = (float)army->soldados / B;

    /*
        Efecto de los iniciados:
        Aporte positivo inicial modulado por e^(-iniciados / K).
        Si supera los 20 iniciados, el desborde aplica penalización progresiva.
    */
    float nIniciados = (float)army->iniciados;
    float aporteIniciados = (nIniciados * 0.6f) * expf(-nIniciados / 18.0f);

    float penalidadExceso = 0.0f;
    if (army->iniciados > 20)
    {
        penalidadExceso = (nIniciados - 20.0f) * 0.45f;
    }

    float totalIniciados = aporteIniciados - penalidadExceso;

    float result = execCommander + valSargentos + valSoldados + totalIniciados;
    return (result > 0.0f) ? result : 0.0f;
}

float Army_CalculateMobility(const Army* army)
{
    if (army == NULL) return 1.0f;

    float baseMob = (float)Commander_GetLeadership(army->commander);
    if (baseMob < 1.0f) baseMob = 1.0f;

    /* El ganado ralentiza la marcha si es muy numeroso */
    float lastreGanado = (float)army->ganado * 0.05f;

    float mob = baseMob - lastreGanado;
    return (mob > 1.0f) ? mob : 1.0f;
}

float Army_CalculateVision(const Army* army)
{
    if (army == NULL) return 1.0f;

    float vis = (float)Commander_GetVision(army->commander);
    if (army->sargentos > 0)
    {
        vis += 1.0f; /* Sargentos actúan como batidores */
    }

    return (vis > 1.0f) ? vis : 1.0f;
}

float Army_CalculateEvaluation(const Army* army)
{
    if (army == NULL) return 0.0f;

    float eval = (float)Commander_GetEvaluation(army->commander);
    eval += (float)army->sargentos * 0.3f;
    return (eval > 0.0f) ? eval : 0.0f;
}

float Army_CalculatePlanning(const Army* army)
{
    if (army == NULL) return 0.0f;

    float plan = (float)Commander_GetPlanning(army->commander);
    plan += (float)army->sargentos * 0.2f;
    return (plan > 0.0f) ? plan : 0.0f;
}

ArmyStats Army_GetStats(const Army* army, float B)
{
    ArmyStats stats;
    stats.vision = Army_CalculateVision(army);
    stats.movilidad = Army_CalculateMobility(army);
    stats.evaluacion = Army_CalculateEvaluation(army);
    stats.planificacion = Army_CalculatePlanning(army);
    stats.ejecucion = Army_CalculateExecution(army, B);
    return stats;
}


/* ============================================================
   SISTEMA DE MOTÍN Y TRAICIÓN
   ============================================================ */

float Army_CalculateMutinyRisk(const Army* army)
{
    if (army == NULL || army->lieutenantCount == 0)
        return 0.0f;

    /* El riesgo solo se activa si la moral es baja (< 50) */
    if (army->moral >= 50.0f)
        return 0.0f;

    float moralDeficit = (50.0f - army->moral) / 50.0f; /* 0.0 a 1.0 */
    float maxLieutenantDisloyalty = 0.0f;

    for (int i = 0; i < army->lieutenantCount; i++)
    {
        int loyalty = army->lieutenants[i].loyalty;
        if (loyalty < 50)
        {
            float disloyalty = (float)(50 - loyalty) / 50.0f;
            if (disloyalty > maxLieutenantDisloyalty)
            {
                maxLieutenantDisloyalty = disloyalty;
            }
        }
    }

    /* Si ningún teniente es desleal, el riesgo es nulo */
    if (maxLieutenantDisloyalty <= 0.0f)
        return 0.0f;

    /* El riesgo porcentual escala con el déficit de moral y la deslealtad */
    float risk = moralDeficit * maxLieutenantDisloyalty * 100.0f;

    /* A mayor infamia, mayor es la fricción interna si la moral es pésima */
    if (army->infamia > 20)
    {
        risk += (float)army->infamia * 0.15f;
    }

    if (risk > 100.0f) risk = 100.0f;
    return risk;
}

bool Army_CheckMutiny(Army* army, int* outRebelLieutenantIndex)
{
    if (army == NULL || army->lieutenantCount == 0)
        return false;

    float risk = Army_CalculateMutinyRisk(army);
    if (risk <= 0.0f)
        return false;

    /* Tirada de azar de 0 a 100 */
    float roll = (float)(rand() % 1000) / 10.0f;

    if (roll < risk)
    {
        /* Encuentra al teniente con menor lealtad que lidera el motín */
        int worstIdx = 0;
        int lowestLoyalty = 100;
        for (int i = 0; i < army->lieutenantCount; i++)
        {
            if (army->lieutenants[i].loyalty < lowestLoyalty)
            {
                lowestLoyalty = army->lieutenants[i].loyalty;
                worstIdx = i;
            }
        }

        if (outRebelLieutenantIndex != NULL)
        {
            *outRebelLieutenantIndex = worstIdx;
        }

        return true;
    }

    return false;
}

int Army_GetVision(const Army* army) {
    if (!army) return 0;
    int vis = 2; // Default base vision is 2
    if (army->commander.type == OFFICER_GENERAL && army->commander.ptr) {
        vis += ((General*)army->commander.ptr)->vision;
    } else if (army->commander.type == OFFICER_LIEUTENANT && army->commander.ptr) {
        vis += ((Lieutenant*)army->commander.ptr)->vision;
    }
    if (vis < 1) vis = 1; // Minimum 1 tile vision
    return vis;
}

