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
* `void Army_ConsumeTurnResources(Army* army);`
  Consume los recursos correspondientes a una franja horaria ($0.25 \times \text{consumo diario}$ por slot, 4 slots por jornada).
* `SupplyForecast Army_ForecastSupply(const Army* army, const Map* map, HexCoord pos);`
  Fuente única de verdad para la proyección de suministros. Simula hacia adelante ranura por ranura con una copia del ejército hasta agotar reservas (tope 30 días) evaluando tres escenarios: acampando, marcha diurna y marcha nocturna. Considera el peor multiplicador de bioma dentro de un radio configurable (`forecastRadius`). Retorna `campDays`, `marchDays` y `nightMarchDays`.
* `float Army_DaysOfSupply(const Army* army);`
  Calcula el remanente mínimo de días de consumo sostenible ($\min(\text{comida}, \text{agua}) / \text{consumo diario}$).

---

## 4. Inteligencia Artificial Enemiga (`models/enemy.h`)

Comportamientos tácticos, estados de la máquina de decisión, etiquetas y zona de control (Sección 8, A5.6).

```c
typedef enum {
    TAG_NONE            = 0,
    TAG_CAVALRY         = 1 << 0,
    TAG_AMBUSH          = 1 << 1,
    TAG_EXPOSED_FLANK   = 1 << 2,
    TAG_HEAVILY_ARMORED = 1 << 3,
    TAG_DISORGANIZED    = 1 << 4,
    TAG_FORTIFIED       = 1 << 5
} EnemyTag;

typedef enum {
    AI_PATROL = 0, /* Patrullaje regular entre waypoints */
    AI_GUARD,      /* Guardia defensivo de estructuras (persigue a corta distancia y retorna) */
    AI_HUNTER,     /* Rastreador orientado a retaguardia/última posición de campamento */
    AI_AMBUSH,     /* Oculto en alta cobertura; sorprende al jugador (ambushed=true) */
    AI_BEAST       /* Depredador agresivo sin formación militar formal */
} EnemyBehavior;

typedef enum {
    AIS_IDLE = 0,    /* Inactivo o apostado en reposo */
    AIS_PATROLLING, /* Recorriendo su ruta preestablecida */
    AIS_ALERT,      /* Sospecha de presencia cercana */
    AIS_CHASING,    /* Persecución directa del ejército jugador */
    AIS_SEARCHING,  /* Explorando última posición conocida tras perder línea de visión */
    AIS_RETURNING,  /* Retorno a su zona base tras agotar ChaseTurns */
    AIS_RESTING     /* En recuperación tras huida/combate (skipNextPhase) */
} EnemyAIState;
```

### Campos clave en `EnemySquad`:
* `behavior`: Estrategia asignada al escuadrón (`AI_PATROL`, `AI_GUARD`, `AI_HUNTER`, `AI_AMBUSH`, `AI_BEAST`).
* `aiState`: Estado actual de la FSM (`AIS_IDLE` .. `AIS_RESTING`).
* `tags`: Banderas de características tácticas activas (`TAG_CAVALRY`, `TAG_FORTIFIED`, etc.).
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
* `int City_GetTempleCurseRemoveCost(const CityContext* ctx, const Army* army);`
  Calcula el costo en oro para purificar una maldición, escalando con cada purificación previa realizada por el ejército: $\text{baseCost} + \text{army->cursesRemovedCount} \times 20$.
* `bool City_RemoveCurse(CityContext* ctx, Army* army);`
* `int City_GetInnCost(const CityContext* ctx);`
* `bool City_RestAtInn(CityContext* ctx, Army* army);`

---

## 6. Facciones y Asedio (`models/city.h`, `controllers/game_master.h`, `models/map_generator.c`)

Sistema sociopolítico y confrontación táctica de asedio (Secciones 9.4 y 9.5, A5.3, A5.4).

* **Asignación Espacial de Facciones:** Cada nivel inicializa 3 semillas espaciales en el mapa; cada cluster urbano toma la facción de la semilla más cercana (diagrama de Voronoi), garantizando coherencia territorial en lugar de distribución modular.

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
  Eleva la alerta a Asedio, inicializa el combate contra una guarnición con `TAG_FORTIFIED` (150 salud, 6 rondas) y fija `activeCombat.lootMultiplier = 3.0f`.
* `void GameMaster_ResolveSiegeVictory(GameMaster* gm, Map* map, Army* playerArmy, HexCoord targetCoord);`
  El botín triplicado es computado y acreditado automáticamente por `Combat_ComputeLoot` y `Combat_Commit` usando `lootMultiplier`. Incrementa infamia ($+15$), descuenta opinión ($-40$), marca `isBesieged = true` y pone en alerta a la facción por 5 días.

---

## 7. Combate Táctico y Widgets de Presentación (`models/combat.h`, `presentation/ui_widgets.h`)

* **Inicialización de Planes (`CombatContext`):** `Combat_Init` inicializa `previousPlan = PLAN_NONE`, garantizando que en la Ronda 1 ningún plan figure con la etiqueta `[FATIGADO]`.
* **Multiplicador de Botín Unificado:** `ctx->lootMultiplier` (por defecto $1.0f$, $3.0f$ en asedio) modula las recompensas de oro y comida de forma centralizada en `Combat_ComputeLoot`.
* **Ciclo de Vida de Widgets UI (`presentation/ui_widgets.h`):**
  * `void UI_Init(void);`
  * `void UI_Shutdown(void);` (liberación segura de memoria de tooltips y temas).
  * `bool UI_ButtonClicked(Rectangle bounds);` (gestión unificada de colisión y clic del mouse).

---

## 8. Glosario, Tooltips y Tarjetas de Plan (`presentation/glossary.h`, `presentation/ui_widgets.h`, `models/combat.h`, `assets/glossary.cfg`)

Sistema unificado de legibilidad de interfaz (Secciones B1, B2 y B3 de `arreglo_correcciones.md`).

* **Glosario Centralizado (`presentation/glossary.h`):** Fuente única de verdad cargada desde `assets/glossary.cfg`.
  * `bool Glossary_Init(const char* filepath);`
  * `void Glossary_Shutdown(void);`
  * `const GlossaryEntry* Glossary_Get(const char* termId);`
  * `bool Glossary_Has(const char* termId);`
  * `bool Glossary_ValidateAll(void);` (Valida en inicialización que los 19 términos mínimos obligatorios estén presentes: TIRADA, PUNTAJE, CHOQUE, MARGEN, INTENCION, FATIGADO, EXPUESTO, FORTIFICADO, DESORGANIZADOS, CABALLERIA, MANDO, FUERA_DE_COMBATE, VENTAJA, DESVENTAJA, MORAL, SALUD, RACIONES, INFAMIA, LEALTAD).
* **Widgets de Tooltip y Chips UI (`presentation/ui_widgets.h`):**
  * `void UI_RequestTooltip(Rectangle triggerArea, const char* title, const char* description, const char* hotkeyHint);`
  * `void UI_RequestGlossaryTooltip(Rectangle triggerArea, const char* termId);`
  * `void UI_DrawPendingTooltips(void);` (Renderizado diferido en la capa superior de la pantalla antes de `EndDrawing()`, con clamp a bordes de pantalla y retardo de hover de 0.35s).
  * `Rectangle UI_DrawChip(Vector2 position, const char* label, Color bg, Color text);`
  * `Rectangle UI_DrawChipEx(Vector2 position, const char* label, const char* glossaryTermId, Color bg, Color text);`
* **Información de Planes de Combate (`models/combat.h`):**
  * `CombatPlanInfo Combat_GetPlanInfo(const CombatContext* ctx, CombatPlan plan);`
    Retorna una estructura `CombatPlanInfo` con nombre, descripción, costo de tirada, estado de bloqueo (`locked`), puntos faltantes (`neededPoints`), chips de efecto (`effectChips`), flecha de ventaja/desventaja frente a la intención revelada del enemigo (`vsIntentAdvantage`: +1 ventaja, -1 desventaja, 0 neutro) y flag `isFatigued`. Usado por `combat_view.c` para renderizar tarjetas interactivas de 2 columnas.


