#include "army.h"
#include "models/modifier.h"
#include "models/balance_config.h"
#include "models/map_generator.h"
#include "models/rng.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>

/* ============================================================
   CONFIGURACIÓN E INICIALIZACIÓN DE LA ARMY
   ============================================================ */

void ArmySetup_InitDefault(ArmySetupConfig* setup) {
    if (!setup) return;
    memset(setup, 0, sizeof(ArmySetupConfig));

    setup->general = General_GetPreset(0);
    setup->unspentStatPoints = 0;

    int ltPresets = Lieutenant_GetPresetCount();
    if (ltPresets > MAX_ARMY_LIEUTENANTS) ltPresets = MAX_ARMY_LIEUTENANTS;
    for (int i = 0; i < ltPresets; i++) {
        setup->lieutenants[i] = Lieutenant_GetPreset(i);
    }
    setup->lieutenantCount = ltPresets;

    setup->sargentos = 8;
    setup->soldados  = 40;
    setup->iniciados = 15;
    setup->ganado    = 12;
    setup->agua      = 1800.0f;
    setup->comida    = 1800.0f;
    setup->monedas   = 350;
}

void Army_Init(Army* army, const ArmySetupConfig* setup, HexCoord startPos) {
    if (!army) return;

    ArmySetupConfig defaultSetup;
    if (!setup) {
        ArmySetup_InitDefault(&defaultSetup);
        setup = &defaultSetup;
    }

    General gen;
    General_Init(&gen, &setup->general);

    *army = Army_Create(
        startPos,
        gen,
        setup->sargentos,
        setup->soldados,
        setup->iniciados,
        setup->agua,
        setup->comida,
        setup->monedas,
        setup->ganado
    );

    for (int i = 0; i < setup->lieutenantCount && i < MAX_ARMY_LIEUTENANTS; i++) {
        Lieutenant lt;
        Lieutenant_Init(&lt, &setup->lieutenants[i]);
        Army_AddLieutenant(army, lt);
    }

    /* Asegurar que la referencia polimórfica apunte a la instancia real en memoria */
    Army_SetCommanderGeneral(army);
}

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
    army.vino_disponible = 0.0f;
    army.carne_disponible = 0.0f;
    army.monedas = (monedas >= 0) ? monedas : 0;
    army.ganado = (ganado >= 0) ? ganado : 0;
    army.commandTokens = 3; /* Comenzar con 3 fichas de mando */

    army.hasDeposedGeneral = false;
    army.mutinyGraceDaysLeft = 0;
    army.warnedMoral1 = false;
    army.warnedMoral2 = false;

    /* Inicializar estados por grupo de raciones (6.3) */
    army.groupState[RG_INICIADOS].morale = 80.0f;
    army.groupState[RG_INICIADOS].health = 100.0f;
    army.groupState[RG_INICIADOS].daysUnfed = 0;
    army.groupState[RG_INICIADOS].satisfaction = 0.0f;
    army.groupState[RG_INICIADOS].satisfactionMax = 0.0f;

    army.groupState[RG_SOLDADOS].morale = 80.0f;
    army.groupState[RG_SOLDADOS].health = 100.0f;
    army.groupState[RG_SOLDADOS].daysUnfed = 0;
    army.groupState[RG_SOLDADOS].satisfaction = 0.0f;
    army.groupState[RG_SOLDADOS].satisfactionMax = 0.0f;

    army.groupState[RG_SARGENTOS].morale = 80.0f;
    army.groupState[RG_SARGENTOS].health = 100.0f;
    army.groupState[RG_SARGENTOS].daysUnfed = 0;
    army.groupState[RG_SARGENTOS].satisfaction = 10.0f;
    army.groupState[RG_SARGENTOS].satisfactionMax = 10.0f;

    army.groupState[RG_GANADO].morale = 0.0f;
    army.groupState[RG_GANADO].health = 100.0f;
    army.groupState[RG_GANADO].daysUnfed = 0;
    army.groupState[RG_GANADO].satisfaction = 0.0f;
    army.groupState[RG_GANADO].satisfactionMax = 0.0f;

    army.hasLastRationPlan = false;
    army.dayRationCommitted = false;

    army.moral = 80.0f;  /* Moral inicial sólida (promedio ponderado) */
    army.salud = 100.0f; /* Salud plena */
    army.infamia = 0;

    return army;
}

int Army_GetTotalSoldiers(const Army* army)
{
    if (army == NULL) return 0;
    return army->sargentos + army->soldados + army->iniciados;
}

int Army_GetTotalWeight(const Army* army) {
    if (!army) return 0;
    return (army->sargentos * 5) + (army->soldados * 2) + (army->iniciados * 1);
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
    (void)army;
    /* Obsoleto: Reemplazado por el sistema de reparto de raciones diario (Ration_Commit).
       El consumo ahora ocurre una única vez al cierre del día. */
}

SupplyForecast Army_ForecastSupply(const Army* army, const Map* map, HexCoord pos) {
    SupplyForecast fc = { 0.0f, 0.0f, 0.0f };
    if (!army) return fc;

    RationNeeds needs = Ration_GetNeeds(army, map, pos);
    float totalDrinkNeed = needs.drink[RG_INICIADOS] + needs.drink[RG_SOLDADOS] +
                           needs.drink[RG_SARGENTOS] + needs.drink[RG_GANADO];
    float totalFoodNeed  = needs.food[RG_INICIADOS] + needs.food[RG_SOLDADOS] +
                           needs.food[RG_SARGENTOS] + needs.food[RG_GANADO];

    float totalDrinkStock = army->agua_disponible + army->vino_disponible;
    float totalFoodStock  = army->comida_disponible + army->carne_disponible;

    float drinkDays = (totalDrinkNeed > 0.001f) ? (totalDrinkStock / totalDrinkNeed) : 999.0f;
    float foodDays  = (totalFoodNeed > 0.001f) ? (totalFoodStock / totalFoodNeed) : 999.0f;
    float days = (drinkDays < foodDays) ? drinkDays : foodDays;

    fc.campDays = days;
    fc.marchDays = days;
    fc.nightMarchDays = days;
    return fc;
}

float Army_DaysOfSupply(const Army* army) {
    if (!army) return 0.0f;
    SupplyForecast fc = Army_ForecastSupply(army, NULL, (HexCoord){0, 0});
    return fc.campDays;
}

void Army_CalculateRankCasualties(int* count, float desertPct, float deathPct, float rankMult, int* outDesert, int* outDead, Rng* rng) {
    *outDesert = 0;
    *outDead = 0;
    if (!count || *count <= 0) return;

    float effDesert = (desertPct * rankMult) / 100.0f;
    float effDeath  = (deathPct  * rankMult) / 100.0f;

    float expDesert = (float)(*count) * effDesert;
    int deserters = (int)expDesert;
    float fracD = expDesert - (float)deserters;
    if (rng && Rng_Float(rng) < fracD) deserters++;
    if (deserters > *count) deserters = *count;

    *count -= deserters;
    *outDesert = deserters;

    float expDeath = (float)(*count) * effDeath;
    int dead = (int)expDeath;
    float fracDead = expDeath - (float)dead;
    if (rng && Rng_Float(rng) < fracDead) dead++;
    if (dead > *count) dead = *count;

    *count -= dead;
    *outDead = dead;
}

void Army_RecalculateAggregates(Army* army) {
    if (!army) return;

    int totalHuman = army->iniciados + army->soldados + army->sargentos;
    if (totalHuman > 0) {
        float weightedMoral = (float)army->iniciados * army->groupState[RG_INICIADOS].morale +
                              (float)army->soldados  * army->groupState[RG_SOLDADOS].morale +
                              (float)army->sargentos * army->groupState[RG_SARGENTOS].morale;
        float weightedSalud = (float)army->iniciados * army->groupState[RG_INICIADOS].health +
                              (float)army->soldados  * army->groupState[RG_SOLDADOS].health +
                              (float)army->sargentos * army->groupState[RG_SARGENTOS].health;
        army->moral = weightedMoral / (float)totalHuman;
        army->salud = weightedSalud / (float)totalHuman;
    } else {
        army->moral = 0.0f;
        army->salud = 0.0f;
    }

    if (army->moral < 0.0f) army->moral = 0.0f;
    if (army->moral > 100.0f) army->moral = 100.0f;
    if (army->salud < 0.0f) army->salud = 0.0f;
    if (army->salud > 100.0f) army->salud = 100.0f;

    int maxUnfed = 0;
    for (int g = RG_INICIADOS; g <= RG_SARGENTOS; g++) {
        if (army->groupState[g].daysUnfed > maxUnfed) {
            maxUnfed = army->groupState[g].daysUnfed;
        }
    }
    army->daysWithoutFood = maxUnfed;
    army->daysWithoutWater = maxUnfed;
}

void Army_ApplyMoraleDelta(Army* army, float delta) {
    if (!army) return;
    for (int g = RG_INICIADOS; g <= RG_SARGENTOS; g++) {
        army->groupState[g].morale += delta;
        if (army->groupState[g].morale < 0.0f) army->groupState[g].morale = 0.0f;
        if (army->groupState[g].morale > 100.0f) army->groupState[g].morale = 100.0f;
    }
    Army_RecalculateAggregates(army);
}

void Army_ApplyHealthDelta(Army* army, float delta) {
    if (!army) return;
    for (int g = RG_INICIADOS; g <= RG_SARGENTOS; g++) {
        army->groupState[g].health += delta;
        if (army->groupState[g].health < 0.0f) army->groupState[g].health = 0.0f;
        if (army->groupState[g].health > 100.0f) army->groupState[g].health = 100.0f;
    }
    Army_RecalculateAggregates(army);
}

void Army_ApplyGroupMoraleDelta(Army* army, RationGroup g, float delta) {
    if (!army || g < 0 || g >= RG_COUNT) return;
    if (g == RG_GANADO) return; /* ganado no tiene moral */
    army->groupState[g].morale += delta;
    if (army->groupState[g].morale < 0.0f) army->groupState[g].morale = 0.0f;
    if (army->groupState[g].morale > 100.0f) army->groupState[g].morale = 100.0f;
    Army_RecalculateAggregates(army);
}

void Army_ApplyGroupHealthDelta(Army* army, RationGroup g, float delta) {
    if (!army || g < 0 || g >= RG_COUNT) return;
    army->groupState[g].health += delta;
    if (army->groupState[g].health < 0.0f) army->groupState[g].health = 0.0f;
    if (army->groupState[g].health > 100.0f) army->groupState[g].health = 100.0f;
    Army_RecalculateAggregates(army);
}

void Army_EndDay(Army* army, DayReport* report, Rng* rng) {
    if (!army) return;
    DayReport localReport;
    if (!report) report = &localReport;

    const BalanceConfig* cfg = Balance_GetGlobal();

    /* Si no se comprometió un reparto de raciones hoy, ejecutar reparto automático (fallback) */
    if (!army->dayRationCommitted) {
        RationNeeds needs = Ration_GetNeeds(army, NULL, army->position);
        RationPlan plan = Ration_Auto(army, &needs);
        Ration_Commit(army, &plan, report, rng);
    }
    army->dayRationCommitted = false;

    /* 6. Efectos de oficiales activos en el ejercito (E3):
          Capellan: +2 moral diaria mientras moral < 50 */
    Lieutenant* chaplain = Army_GetLieutenantByRole(army, ROLE_CAPELLAN);
    if (chaplain && army->moral < 50.0f) {
        Army_ApplyMoraleDelta(army, 2.0f);
        if (army->moral > 50.0f) {
            float excess = army->moral - 50.0f;
            Army_ApplyMoraleDelta(army, -excess);
        }
        report->moralDelta += 2.0f;
    }

    /* 7. Deriva diaria de lealtad si moral < 30 (E3) */
    if (army->moral < 30.0f) {
        int drift = (cfg && cfg->dailyDriftWhenMoralBelow30 != 0) ? cfg->dailyDriftWhenMoralBelow30 : -1;
        for (int i = 0; i < army->lieutenantCount; i++) {
            int nl = army->lieutenants[i].loyalty + drift;
            if (nl < 0) nl = 0;
            army->lieutenants[i].loyalty = nl;
        }
    }

    /* 8. Periodo de gracia tras motin (evita bucles, E1) */
    if (army->mutinyGraceDaysLeft > 0) {
        army->mutinyGraceDaysLeft--;
    } else {
        /* 9. Chequeo de motin diario a moral <= CheckMoral (E2) */
        float checkMoral = (cfg && cfg->checkMoral > 0.0f) ? cfg->checkMoral : 10.0f;
        if (army->moral <= checkMoral) {
            int rebelIdx = -1;
            if (Army_CheckMutiny(army, &rebelIdx)) {
                Army_TriggerMutiny(army, rebelIdx);
                report->mutinyOccurred = true;
            } else if (army->lieutenantCount == 0) {
                /* Si no quedan tenientes, ocurre una desercion masiva (E1) */
                int desIniciados = army->iniciados / 2;
                int desSoldados = army->soldados / 2;
                int desSargentos = army->sargentos / 2;
                army->iniciados -= desIniciados;
                army->soldados -= desSoldados;
                army->sargentos -= desSargentos;
                report->desertedInitiates += desIniciados;
                report->desertedSoldiers += desSoldados;
                report->desertedSergeants += desSargentos;
            }
        }
    }

    /* 10. Avisos telegrafiados de motin (E2) */
    if (cfg) {
        if (army->moral <= cfg->warnMoral2 && !army->warnedMoral2) {
            army->warnedMoral2 = true;
            report->triggerWarn2 = true;
        } else if (army->moral <= cfg->warnMoral1 && !army->warnedMoral1) {
            army->warnedMoral1 = true;
            report->triggerWarn1 = true;
        }

        /* Recuperacion de advertencias si la moral mejora */
        if (army->moral > cfg->warnMoral1) {
            army->warnedMoral1 = false;
            army->warnedMoral2 = false;
        } else if (army->moral > cfg->warnMoral2) {
            army->warnedMoral2 = false;
        }
    }
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

bool Army_TriggerMutiny(Army* army, int rebelLieutenantIndex) {
    if (!army) return false;
    if (rebelLieutenantIndex < 0 || rebelLieutenantIndex >= army->lieutenantCount) return false;

    /* 1. Si el comandante actual es el General, preservarlo en deposedGeneral con sus cartas y maldiciones */
    if (army->commander.type == OFFICER_GENERAL) {
        army->deposedGeneral = army->general;
        army->hasDeposedGeneral = true;
    }

    /* 2. El teniente rebelde asume el mando */
    Lieutenant* rebel = &army->lieutenants[rebelLieutenantIndex];
    army->commander = Commander_FromLieutenant(rebel);

    /* Activar su carta patrona como su unica carta activa si tiene una definida */
    if (strlen(rebel->patronGod) > 0 && !rebel->hasPatronCard) {
        /* Si patronCard.id ya tiene valor, marcar como activa */
        if (strlen(rebel->patronCard.id) > 0) {
            rebel->hasPatronCard = true;
            rebel->patronCard.remaining = -1;
        }
    }

    /* 3. Ajustar moral y periodo de gracia segun configuracion */
    const BalanceConfig* cfg = Balance_GetGlobal();
    float moralAfter = (cfg && cfg->moralAfterMutiny > 0.0f) ? cfg->moralAfterMutiny : 35.0f;
    int grace = (cfg && cfg->graceDays > 0) ? cfg->graceDays : 3;

    army->moral = moralAfter;
    army->mutinyGraceDaysLeft = grace;
    army->warnedMoral1 = false;
    army->warnedMoral2 = false;

    return true;
}

bool Army_RestoreGeneral(Army* army) {
    if (!army || !army->hasDeposedGeneral) return false;

    /* Restaurar el general preservado */
    army->general = army->deposedGeneral;
    army->hasDeposedGeneral = false;

    /* Si el comandante actual era un teniente, reducir su lealtad en 20 */
    if (army->commander.type == OFFICER_LIEUTENANT && army->commander.ptr) {
        Lieutenant* lt = (Lieutenant*)army->commander.ptr;
        lt->loyalty -= 20;
        if (lt->loyalty < 0) lt->loyalty = 0;
    }

    /* El general vuelve al mando */
    army->commander = Commander_FromGeneral(&army->general);
    return true;
}

void Army_DismissGeneral(Army* army) {
    if (!army) return;
    /* El general queda descartado definitivamente */
    army->hasDeposedGeneral = false;
}

Lieutenant* Army_GetLieutenantByRole(Army* army, OfficerRole role) {
    if (!army || role == ROLE_NONE) return NULL;
    for (int i = 0; i < army->lieutenantCount; i++) {
        if (army->lieutenants[i].role == role) {
            return &army->lieutenants[i];
        }
    }
    return NULL;
}

void Army_ApplyAdviceEffect(Army* army, OfficerRole role, int loyaltyDelta) {
    Lieutenant* lt = Army_GetLieutenantByRole(army, role);
    if (lt) {
        lt->loyalty += loyaltyDelta;
        if (lt->loyalty < 0) lt->loyalty = 0;
        if (lt->loyalty > 100) lt->loyalty = 100;
    }
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

EquipResult Army_EquipModifier(Army* army, const ModifierDef* def) {
    if (!army || !def) return EQUIP_INVALID;
    if (Army_HasModifier(army, def->id)) return EQUIP_DUPLICATE;
    int remaining = -1;
    if (def->durationType == DURATION_BATTLES || def->durationType == DURATION_DAYS) remaining = def->durationValue;
    
    if (def->target == MOD_TARGET_COMMANDER && def->kind == MOD_KIND_CARD) {
        if (army->commander.type == OFFICER_GENERAL) {
            General* g = (General*)army->commander.ptr;
            if (g->cardCount >= 3) return EQUIP_SLOTS_FULL;
            strncpy(g->cards[g->cardCount].id, def->id, 31);
            g->cards[g->cardCount].remaining = remaining;
            g->cardCount++;
            return EQUIP_OK;
        } else {
            Lieutenant* lt = (Lieutenant*)army->commander.ptr;
            if (lt->hasPatronCard) return EQUIP_SLOTS_FULL;
            strncpy(lt->patronCard.id, def->id, 31);
            lt->patronCard.remaining = remaining;
            lt->hasPatronCard = true;
            return EQUIP_OK;
        }
    } else if (def->target == MOD_TARGET_COMMANDER && def->kind == MOD_KIND_CURSE) {
        if (army->commander.type == OFFICER_GENERAL) {
            General* g = (General*)army->commander.ptr;
            if (g->curseCount >= 4) return EQUIP_SLOTS_FULL;
            strncpy(g->curses[g->curseCount].id, def->id, 31);
            g->curses[g->curseCount].remaining = remaining;
            g->curseCount++;
            return EQUIP_OK;
        } else {
            Lieutenant* lt = (Lieutenant*)army->commander.ptr;
            if (lt->curseCount >= 4) return EQUIP_SLOTS_FULL;
            strncpy(lt->curses[lt->curseCount].id, def->id, 31);
            lt->curses[lt->curseCount].remaining = remaining;
            lt->curseCount++;
            return EQUIP_OK;
        }
    } else if (def->target == MOD_TARGET_ARMY) {
        if (army->curseCount >= 8) return EQUIP_SLOTS_FULL;
        strncpy(army->curses[army->curseCount].id, def->id, 31);
        army->curses[army->curseCount].remaining = remaining;
        army->curseCount++;
        return EQUIP_OK;
    }
    return EQUIP_INVALID;
}

bool Army_ReplaceCard(Army* army, int slot, const ModifierDef* def) {
    if (!army || !def || def->kind != MOD_KIND_CARD) return false;
    int remaining = -1;
    if (def->durationType == DURATION_BATTLES || def->durationType == DURATION_DAYS) remaining = def->durationValue;
    
    if (army->commander.type == OFFICER_GENERAL) {
        General* g = (General*)army->commander.ptr;
        if (slot < 0 || slot >= 3) return false;
        strncpy(g->cards[slot].id, def->id, 31);
        g->cards[slot].remaining = remaining;
        if (slot >= g->cardCount) g->cardCount = slot + 1;
        return true;
    } else {
        Lieutenant* lt = (Lieutenant*)army->commander.ptr;
        if (slot != 0) return false;
        strncpy(lt->patronCard.id, def->id, 31);
        lt->patronCard.remaining = remaining;
        lt->hasPatronCard = true;
        return true;
    }
}

bool Army_RemoveModifier(Army* army, const char* id) {
    if (!army || !id) return false;
    if (army->commander.type == OFFICER_GENERAL) {
        General* g = (General*)army->commander.ptr;
        for (int i = 0; i < g->cardCount; i++) {
            if (strcmp(g->cards[i].id, id) == 0) {
                for (int j = i; j < g->cardCount - 1; j++) g->cards[j] = g->cards[j+1];
                g->cardCount--; return true;
            }
        }
        for (int i = 0; i < g->curseCount; i++) {
            if (strcmp(g->curses[i].id, id) == 0) {
                for (int j = i; j < g->curseCount - 1; j++) g->curses[j] = g->curses[j+1];
                g->curseCount--; return true;
            }
        }
    } else {
        Lieutenant* lt = (Lieutenant*)army->commander.ptr;
        if (lt->hasPatronCard && strcmp(lt->patronCard.id, id) == 0) { lt->hasPatronCard = false; return true; }
        for (int i = 0; i < lt->curseCount; i++) {
            if (strcmp(lt->curses[i].id, id) == 0) {
                for (int j = i; j < lt->curseCount - 1; j++) lt->curses[j] = lt->curses[j+1];
                lt->curseCount--; return true;
            }
        }
    }
    for (int i = 0; i < army->curseCount; i++) {
        if (strcmp(army->curses[i].id, id) == 0) {
            for (int j = i; j < army->curseCount - 1; j++) army->curses[j] = army->curses[j+1];
            army->curseCount--; return true;
        }
    }
    return false;
}

void Army_TickModifiers(Army* army, enum DurationType type) {
    // Phase B tick implementation placeholder
}

bool Army_HasModifier(const Army* army, const char* id);
bool Army_HasModifier(const Army* army, const char* id) {
    if (!army || !id) return false;
    if (army->commander.type == OFFICER_GENERAL) {
        General* g = (General*)army->commander.ptr;
        for (int i = 0; i < g->cardCount; i++) if (strcmp(g->cards[i].id, id) == 0) return true;
        for (int i = 0; i < g->curseCount; i++) if (strcmp(g->curses[i].id, id) == 0) return true;
    } else {
        Lieutenant* lt = (Lieutenant*)army->commander.ptr;
        if (lt->hasPatronCard && strcmp(lt->patronCard.id, id) == 0) return true;
        for (int i = 0; i < lt->curseCount; i++) if (strcmp(lt->curses[i].id, id) == 0) return true;
    }
    for (int i = 0; i < army->curseCount; i++) if (strcmp(army->curses[i].id, id) == 0) return true;
    return false;
}
