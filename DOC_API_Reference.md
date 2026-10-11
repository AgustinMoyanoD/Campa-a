# Referencia de API del Proyecto (DOC_API_Reference.md)

Este documento describe las interfaces y contratos públicos de los módulos añadidos y ampliados durante la reestructuración del ciclo de exploración, supervivencia, IA enemiga, asentamientos, facciones y asedio.

---

## 1. Módulo de Reloj y Ciclo de Tiempo (`models/day_clock.h`)

Controlador unívoco del avance temporal canónico (Invariante I4).

```c
typedef enum {
    TIME_MORNING = 0,
    TIME_MIDDAY,
    TIME_AFTERNOON,
    TIME_NIGHT
} TimeOfDay;

typedef struct {
    int currentDay;             /* Día actual (inicia en 1) */
    TimeOfDay timeOfDay;        /* Franja horaria actual */
    int totalTurnsElapsed;      /* Turnos/bloques de PA totales transcurridos */
    bool dayEndedThisSlot;      /* Flag de cierre de jornada diario */
} DayClock;
```

### Funciones
* `void DayClock_Init(DayClock* clock);`
  Inicializa el reloj en el Día 1, Mañana.
* `void DayClock_AdvanceSlot(DayClock* clock);`
  Avanza una franja horaria. Si se completa la Noche, incrementa el día y activa `dayEndedThisSlot`.
* `bool DayClock_HasDayEnded(DayClock* clock);`
  Consulta si acaba de finalizar un ciclo completo de 4 franjas horarias y limpia el flag para que el evento se procese una sola vez.

---

## 2. Configuración de Balance (`models/balance_config.h`)

Carga centralizada de parámetros desde `assets/balance.cfg` (Invariante I2).

### Funciones
* `void Balance_InitDefaults(BalanceConfig* cfg);`
  Carga los valores de balance de fábrica en la estructura.
* `bool Balance_LoadFromFile(BalanceConfig* cfg, const char* filepath);`
  Parsea el archivo INI de balance especificado.
* `const BalanceConfig* Balance_GetGlobal(void);`
  Retorna la instancia global singleton cargada en memoria.
* `int Balance_GetTerrainCover(const BalanceConfig* cfg, int biome, int structType);`
  Calcula el bono de cobertura para tiradas de campamento según el terreno y estructura.

---

## 3. Supervivencia y Ejército (`models/army.h`)

Contabilidad de raciones y escalera de consecuencias por inanición/sed (Decisión E1).

```c
typedef struct {
    int daysWithoutFood;
    int daysWithoutWater;
    int consecutiveNightMarches;
    ...
} Army;

typedef struct {
    int day;
    bool foodShortage;
    bool waterShortage;
    int daysWithoutFood;
    int daysWithoutWater;
    float moralDelta;
    float saludDelta;
    int deadInitiates, deadSoldiers, deadSergeants;
    int desertedInitiates, desertedSoldiers, desertedSergeants;
} DayReport;
```

### Funciones
* `void Army_EndDay(Army* army, DayReport* report, Rng* rng);`
  Aplica la escalera de inanición y deshidratación al cerrar el día. Genera el informe `DayReport`.
* `float Army_DaysOfSupply(const Army* army);`
  Calcula el remanente mínimo de días de consumo sostenible ($\min(\text{comida}, \text{agua}) / \text{consumo diario}$).

---

## 4. Inteligencia Artificial Enemiga (`models/enemy.h`)

Comportamientos tácticos y zona de control (Sección 8).

```c
typedef enum {
    AI_PATROL = 0,
    AI_GUARD,
    AI_HUNTER,
    AI_AMBUSH,
    AI_BEAST
} EnemyBehavior;

typedef enum {
    AIS_IDLE = 0,
    AIS_PATROLLING,
    AIS_ALERT,
    AIS_CHASING,
    AIS_SEARCHING,
    AIS_RETURNING,
    AIS_RESTING
} EnemyAIState;
```

### Campos clave en `EnemySquad`:
* `behavior`: Estrategia asignada al escuadrón.
* `aiState`: Estado actual de la máquina de estados.
* `mobility`: Puntos de movimiento/PA disponibles por turno.
* `chaseTurnsLeft`: Turnos restantes antes de desistir de la persecución.
* `skipNextPhase`: Turnos que el enemigo debe saltar tras huida o combate.
* `initiator`: `0 = JUGADOR`, `1 = ENEMIGO`.

---

## 5. Ciudades, Mercado, Armería y Servicios (`models/city.h`)

Gestión de tiendas deterministas, promociones de tropas y servicios de salud/moral (Sección 9).

```c
typedef enum {
    CITY_TAB_MAIN = 0,
    CITY_TAB_MARKET,
    CITY_TAB_ARMORY,
    CITY_TAB_HOSPITAL,
    CITY_TAB_TEMPLE,
    CITY_TAB_INN
} CityTab;

typedef struct {
    HexCoord tileCoord;
    StructureType structType;
    SettlementType settlementType;
    int settlementId;
    int factionId;
    char factionName[32];
    int factionOpinion;
    float factionPriceMult;
    int factionAlertLevel;
    bool isBesieged;
    bool requestSiege;
    ...
} CityContext;
```

### Funciones del Mercado:
* `void City_InitVisit(CityContext* ctx, HexCoord coord, StructureType structType, SettlementType settlementType, int settlementId, int currentDay);`
  Genera el stock determinista para la visita basado en la coordenada, ID de asentamiento y día.
* `int City_GetBuyPriceFood(const CityContext* ctx);` (y equivalentes para agua, ganado, iniciados).
* `int City_GetSellPriceFood(const CityContext* ctx);` (precios con ratio de venta del $40\%$).
* `bool City_BuyFood(CityContext* ctx, Army* army, int amount);`
* `bool City_SellFood(CityContext* ctx, Army* army, int amount);`
* `bool City_RecruitInitiates(CityContext* ctx, Army* army, int amount);`

### Funciones de la Armería:
* `int City_GetMaxPromoteInitiates(const CityContext* ctx, const Army* army);`
* `int City_GetMaxPromoteSoldiers(const CityContext* ctx, const Army* army);`
  Verifica fondos, tropas disponibles, tope de 2 por visita y ratio estricto $\text{Sargentos} \le \text{Soldados}/15$.
* `bool City_PromoteInitiates(CityContext* ctx, Army* army, int amount);`
* `bool City_PromoteSoldiers(CityContext* ctx, Army* army, int amount);`

### Funciones de Servicios:
* `int City_GetHospitalCost(const CityContext* ctx, const Army* army, int pointsToHeal);`
* `bool City_HealArmy(CityContext* ctx, Army* army, int pointsToHeal);`
* `int City_GetTempleCurseRemoveCost(const CityContext* ctx);`
* `bool City_RemoveCurse(CityContext* ctx, Army* army);`
* `int City_GetInnCost(const CityContext* ctx);`
* `bool City_RestAtInn(CityContext* ctx, Army* army);`

---

## 6. Facciones y Asedio (`models/city.h`, `controllers/game_master.h`)

Sistema sociopolítico y confrontación táctica de asedio (Secciones 9.4 y 9.5).

```c
typedef enum {
    FACTION_HOSTILE = 0,
    FACTION_SUSPICIOUS,
    FACTION_NEUTRAL,
    FACTION_FRIENDLY,
    FACTION_ALLIED
} FactionOpinionLevel;

typedef struct {
    int id;
    char name[32];
    int temperament; /* -1 odia infamia, +1 la respeta, 0 neutral */
    int opinion;     /* -100..100 */
    int alertLevel;  /* 0: calma, 1: alerta, 2: asedio */
    int alertDays;   /* días restantes de alerta */
} Faction;
```

### Funciones de Facciones:
* `void Faction_Init(Faction* f, int id, const char* name, int temperament, float currentInfamy, const BalanceConfig* cfg);`
  Calcula la opinión inicial: $\text{baseline} + \text{temperament} \times (\text{infamia} - 50) \times k$.
* `FactionOpinionLevel Faction_GetLevel(int opinion, const BalanceConfig* cfg);`
* `float Faction_GetPriceMultiplier(int opinion, const BalanceConfig* cfg);`
* `void Faction_ModifyOpinion(Faction* f, int delta);`
* `void Faction_SetAlert(Faction* f, int alertLevel, int days);`
* `void Faction_TickDay(Faction* f);`

### Funciones de Asedio (`GameMaster`):
* `void GameMaster_StartSiege(GameMaster* gm, Map* map, Army* playerArmy, HexCoord targetCoord);`
  Eleva la alerta a Asedio, inicializa el combate contra una guarnición con `TAG_FORTIFIED` (150 salud, 6 rondas).
* `void GameMaster_ResolveSiegeVictory(GameMaster* gm, Map* map, Army* playerArmy, HexCoord targetCoord);`
  Acredita botín triplicado ($\times 3.0$), incrementa infamia ($+15$), descuenta opinión ($-40$), marca `isBesieged = true` y pone en alerta a todos los centros de la facción durante 5 días.
