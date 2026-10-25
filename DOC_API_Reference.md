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

### Campos Destacados de Balance (Bloque C)
* `initialSupplyDays`: Raciones iniciales de suministro (por defecto `5.0` días).
* `siegeSupplyDays`: Botín mínimo garantizado en días de comida tras vencer un asedio (`5.0` días).
* `siegeBaseGold`: Botín mínimo garantizado en oro tras asedio (`200` oro).
* `nightMoveCostMult`, `nightMoralPerTurn`, `nightSaludPerTurn`: Parámetros retuneados de marcha nocturna (C4).

### Herramientas de Simulación (Headless)
* `tests/sim_crossing.c`: Simulador procedural de cruce de nivel ($N \ge 1.000$) con autopilot, desvíos comerciales, combate y evaluación de raciones iniciales (3, 4, 5, 6 días).
* `tests/sim_night_policy.c`: Simulador de políticas nocturnas (acampar vs marcha vs mixta) con análisis de sensibilidad ($\pm 50\%$) y cartas divinas.

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

---

## 9. Descubrimiento de Asentamientos y Postes Indicadores (`models/map_generator.h`, `views/map_view.h`, `assets/balance.cfg`)

Implementación del Bloque D de `arreglo_correcciones.md`.

* **Garantía Procedural de Asentamientos (D1):**
  * Parámetros en `assets/balance.cfg`: sección `[MAPGEN]` con `MaxDaysToSettlement=2.0` y `HexPerDay=9` (distancia máxima vial = $18$ hexágonos).
  * `void Map_EnsureSettlementGuarantee(Map* map, HexTile* start, int provinceLevel);`
    Evalúa cada casilla de la red vial en todas las rutas de Inicio (A) a cada Salida (B). Si alguna casilla excede la distancia vial máxima a un asentamiento o resulta inalcanzable, inserta de forma determinista `STRUCT_TOWN_HOUSES` (`SETTLEMENT_SECONDARY`) en los puntos críticos de la ruta hasta garantizar el 100% de cobertura.
* **Postes Indicadores y Rumores (D2):**
  * Nueva estructura `STRUCT_SIGNPOST` añadida a `StructureType`.
  * `void Map_GenerateSignposts(Map* map);`
    Genera postes indicadores en todas las bifurcaciones y cruces de caminos ($\ge 3$ vecinos viales) y en tramos rectos largos cada $\sim 8$ hexágonos de camino.
  * Revelación en niebla: Al ver (`visible = true`) o transitar por un poste indicador, se ejecuta un escaneo por red vial hacia el asentamiento más cercano. Si este aún no fue descubierto (`!visited`), se marca con `isRumor = true` sin costo de recursos.
  * Promoción: Al entrar en el campo de visión del jugador o visitarse directamente, el marcador de rumor se promueve a estructura normal descubierta (`isRumor = false`).
  * Renderizado en minimapa: Los rumores se visualizan en el minimapa como rombos ámbar destacados.
* **Indicadores Direccionales Fuera de Pantalla (D3):**
  * `void MapView_DrawOffscreenRumors(const Map* map, Camera2D camera, float radius, Vector2 origin, HexCoord playerPos);`
    Cuando un rumor no está dentro del encuadre actual de la cámara, proyecta su posición y renderiza en el margen de la pantalla una flecha direccional dorada con una insignia que indica la distancia estimada en días (`Rumor: X.X d`).
* **Verificación Automatizada:**
  * `tests/test_settlement_guarantee.c`: Valida $1.000$ mapas procedimentales ($3.000$ rutas y $>55.000$ casillas evaluadas). Cero violaciones de la garantía (distancia máxima $\le 18$), generación promedio de $\sim 34.8$ postes/mapa y verificación de ciclo completo de rumor (revelación y promoción).

---

## 10. Subsistema de Audio e Integración Sonora (`presentation/audio_manager.h`, `assets/audio.cfg`, `assets/audio_manifest.md`)

Gestor centralizado de audio del juego en capa de presentación. Cumple con la arquitectura desacoplada: `models/` no conoce Raylib ni audio; todos los eventos se disparan desde controladores o vistas (`Sequence`, `CombatView`, `CityView`, `DraftView`).

### 10.1. Pistas Musicales y Contextos (`AudioMusicContext`)
Controla la reproducción continua de 4 pistas en streaming (`menu_theme`, `main_theme`, `combat_theme`, `city_theme`).

```c
typedef enum {
    MUSIC_CTX_NONE = 0,
    MUSIC_CTX_MENU,     /* menu_theme: Menú principal de inicio */
    MUSIC_CTX_MAIN,     /* main_theme: Exploración del mapa */
    MUSIC_CTX_COMBAT,   /* combat_theme: Pantalla de combate táctico */
    MUSIC_CTX_CITY,     /* city_theme: Asentamientos y ciudades */
    MUSIC_CTX_COUNT
} AudioMusicContext;
```

* **Transiciones con Fundido (`FadeSeconds`):** Las pistas nunca sufren cortes secos. El cambio de contexto ejecuta un fundido cruzado gradual configurado en `assets/audio.cfg`.
* **Retención de Exploración (Pausa y Reanudación):** Al entrar en combate o ciudad desde el mapa, `main_theme` se pausa (`PauseMusicStream`) en su posición exacta. Al regresar al mapa de exploración, retoma donde quedó con un fundido suave.
* **Bucle Automático:** Si una pista finaliza y el contexto no ha cambiado, se repite desde el inicio en streaming.

### 10.2. Efectos de Sonido (`SfxId`)
Identificadores de efectos separados por categoría de botón/acción con control de saturación:

```c
typedef enum {
    /* Imprescindibles */
    SFX_COMBAT_START = 0,
    SFX_COMBAT_END,
    SFX_DAMAGE_DEALT,
    SFX_MARKET_BUY,
    SFX_DIVINE_CARD_ACQUIRED,

    /* Interfaz, Botones por Categoría y Dados */
    SFX_UI_CLICK,       /* ui_click: Navegación genérica (pestañas, paneles, volver) */
    SFX_MENU_CLICK,     /* menu_click: Botones del menú principal */
    SFX_UI_CONFIRM,     /* ui_confirm: Confirmación de fases tácticas, reportes y modales */
    SFX_UI_HOVER,       /* ui_hover: Pasar el cursor sobre botones */
    SFX_DICE_ROLL,
    SFX_DICE_LAND,

    /* Movimiento, Acciones de Mapa y Campaña */
    SFX_STEP,
    SFX_TURN_END,
    SFX_FORAGE,         /* forage: Botón Forrajear (F) en HUD */
    SFX_CAMP_REST,      /* camp_rest: Elegir Acampar en la tarde */
    SFX_NIGHT_MARCH,    /* night_march: Elegir Marcha Nocturna en la tarde */
    SFX_NIGHT_FALL,
    SFX_EVENT_OPEN,
    SFX_EVENT_CHOICE,   /* event_choice: Elegir una opción de evento narrativo */

    /* Combate Táctico */
    SFX_DAMAGE_TAKEN,
    SFX_CRITICAL_HIT,

    /* Economía, Servicios de Ciudad y Altar */
    SFX_MARKET_SELL,
    SFX_MARKET_DENIED,
    SFX_ALTAR_OFFER,    /* altar_offer: Ofrendar oro en el altar divino */
    SFX_CURSE_ACQUIRED,
    SFX_CARD_SACRIFICED,
    SFX_PROMOTE_TROOPS,
    SFX_HEAL,
    SFX_INN_REST,       /* inn_rest: Descansar y subir moral en la Posada */

    /* Eventos Mayores y Alertas */
    SFX_SIEGE_START,
    SFX_SIEGE_VICTORY,
    SFX_ENEMY_ALERT,
    SFX_LOW_SUPPLY_WARNING,
    SFX_MUTINY_WARNING,
    SFX_LEVEL_COMPLETE,
    SFX_GAME_OVER,

    /* Efectos Ambientales (Aleatorios en mapa de exploracion) */
    SFX_AMBIENT_CROW,
    SFX_AMBIENT_WIND,
    SFX_AMBIENT_BIRDS,
    SFX_AMBIENT_INSECTS,
    SFX_AMBIENT_ANIMALS,

    SFX_COUNT
} SfxId;
```

### 10.3. Funciones Públicas de la API (`presentation/audio_manager.h` y `presentation/ui_widgets.h`)
* `void Audio_Init(void);`
  Inicializa el dispositivo de audio de Raylib, carga `assets/audio.cfg` y ejecuta la auditoría de inicio listando en el log los archivos esperados ausentes.
* `void Audio_Shutdown(void);`
  Descarga todos los streams de música y el caché de sonidos de forma segura antes de cerrar el dispositivo.
* `void Audio_Update(float dt);`
  Actualiza los streams activos de música en cada frame, computa fundidos y descuenta el temporizador ambiental aleatorio en mapa táctico.
* `void Audio_SetMusicContext(AudioMusicContext ctx);`
  Modifica el contexto musical activo desencadenando el fundido progresivo o la reanudación de `main_theme`.
* `AudioMusicContext Audio_GetMusicContext(void);`
  Consulta el contexto musical actualmente en reproducción.
* `void Audio_PlaySfx(SfxId id);`
  Dispara un efecto por su identificador tipado.
* `void Audio_PlaySfxByName(const char* name);`
  Dispara un efecto por su nombre lógico en `assets/audio.cfg`.
* `void Audio_PlayCombatEnd(int endReason);`
  Evalúa las variantes de victoria (`combat_end_victory`), derrota (`combat_end_defeat`) y huida (`combat_end_flee`), con fallback automático a `combat_end`.
* `void Audio_PlayStepBiome(int biome);`
  Prueba la variante del bioma actual (`step_plains`, `step_forest`, etc.) con fallback automático a `step_move`.
* `void Audio_SetVolumes(float master, float sfx, float music);`
  Ajusta en tiempo de ejecución los multiplicadores de volumen maestro, efectos y música.
* `void Audio_SetAmbientTimeOfDay(TimeOfDay tod);`
  Informa la franja horaria actual al subsistema de audio para filtrar los sonidos ambientales pertinentes (aves de día, grillos de noche, viento/fauna).
* `void Audio_TriggerRandomAmbient(void);`
  Sortea y reproduce un efecto ambiental disponible según el momento del día y los archivos presentes en disco.
* `bool UI_DrawButtonSfx(Rectangle bounds, const char* text, bool disabled, const char* sfxOnClick);`
* `bool UI_DrawButtonExSfx(Rectangle bounds, const char* text, int fontSize, bool disabled, Color bgOverride, Color textOverride, Color borderOverride, const char* sfxOnClick);`
  Permiten especificar qué efecto de sonido dispara un botón al pulsarse (p. ej. `"menu_click"`, `"altar_offer"`), o pasar `NULL` cuando la acción posterior dispara su propio sonido transaccional (`market_buy`, `market_sell`, `market_denied`, `divine_card_acquired`, `promote_troops`, `heal`, `inn_rest`, `siege_start`), evitando que `ui_click` suene encima.

### 10.4. Reglas y Arquitectura de Ejecución
1. **Resolución de Archivos:** Las rutas se escriben sin extensión en configuración y código. El motor prueba `.mp3` $\to$ `.ogg` $\to$ `.wav` en `assets/audio/sfx/`, `assets/sfx/`, `assets/audio/music/` y `assets/music/` (priorizando el archivo más reciente si existiera en ambas ubicaciones).
2. **Tolerancia a Archivos Ausentes:** Si un archivo falta en disco, el juego permanece en silencio sin fallar ni interrumpir la ejecución, y registra la ausencia una única vez en el log.
3. **Control de Saturación:**
   * `MinIntervalMs`: Intervalo mínimo en milisegundos para evitar repeticiones excesivas en efectos rápidos (cursor, pasos).
   * **Tono Natural:** Los efectos se reproducen fielmente en su tono original (`pitch = 1.0f`), sin alteraciones ni distorsiones de frecuencia.
4. **Sincronización con Secuencias Visuales:** Los sonidos de combate (`damage_dealt`, `damage_taken`, `combat_end`) se despachan como callbacks de `SeqAction` dentro de `Sequence_Push` en `CombatView_Update`, garantizando sincronización exacta con los números flotantes y las animaciones.

---

## 11. Sistema de Reparto de Raciones e Interfaz (`views/ration_view.h`, `presentation/ui_widgets.h`, `views/ration_mock.h`)

Arquitectura desacoplada de la interfaz de distribución diaria de provisiones entre grupos del ejército (Iniciados, Soldados, Sargentos y Ganado), con soporte de sustitución de recursos (Vino sustituye Agua 1:1, Carne sustituye Comida 1:1), arrastrar y soltar con paquetes, pronóstico en vivo y modo compacto de abundancia.

### 11.1. Contrato de Datos y Tipos
```c
typedef enum { RG_INICIADOS, RG_SOLDADOS, RG_SARGENTOS, RG_GANADO, RG_COUNT } RationGroup;
typedef enum { RS_AGUA, RS_VINO, RS_COMIDA, RS_CARNE, RS_COUNT } RationResource;
typedef struct { float amount[RG_COUNT][RS_COUNT]; } RationPlan;
typedef struct { float drink[RG_COUNT]; float food[RG_COUNT]; } RationNeeds;
typedef enum { RATION_FULL, RATION_REDUCED, RATION_STARVING } RationLevel;
typedef struct { RationLevel level; float moraleDelta, healthDelta, satisfactionDelta; int daysUnfed; bool risksDesertion; } GroupPreview;
typedef struct { GroupPreview group[RG_COUNT]; float stockAfter[RS_COUNT]; float daysLeft[RS_COUNT]; } RationPreview;
typedef struct { float morale, health; int daysUnfed; float satisfaction, satisfactionMax; } GroupState;
```

### 11.2. Componente de Arrastre y Soltar (`presentation/ui_widgets.h`)
Máquina de estados pura para operaciones de Drag and Drop sin efectos colaterales de renderizado:
```c
typedef struct {
    bool active;
    int sourceKind;       /* RationResource o tipo de ítem */
    int sourceGroup;      /* RationGroup de origen (-1 si proviene de pila general) */
    float amount;         /* Cantidad en el paquete arrastrado */
    Vector2 currentPos;   /* Posición actual del ratón */
    char label[32];
    Color color;
} UIDragDrop;

void UIDragDrop_Init(UIDragDrop* dd);
bool UIDragDrop_Start(UIDragDrop* dd, int sourceKind, int sourceGroup, float amount, Vector2 mouse, const char* label, Color color);
void UIDragDrop_Update(UIDragDrop* dd, Vector2 mouse);
void UIDragDrop_Cancel(UIDragDrop* dd);
bool UIDragDrop_Finish(UIDragDrop* dd, int* outSourceKind, int* outSourceGroup, float* outAmount);
void UIDragDrop_DrawFloating(const UIDragDrop* dd);
```
* **Modificadores de Paquete:** Arrastre estándar = `5u`. Con `Shift` sostenido = paquete grande (`20u`). Con `Ctrl` sostenido = paquete unitario (`1u`).
* **Cancelación:** Se cancela automáticamente al pulsar `ESC` o clic secundario derecho.

### 11.3. Ciclo de Vida de la Vista de Raciones (`views/ration_view.h`)
* `void RationView_Init(RationViewState* state, const Army* army, const Map* map, HexCoord pos);`
  Inicializa el borrador del plan intentando repetir el de ayer (`Ration_Repeat`); si no es válido, asigna automático (`Ration_Auto`). Si `Ration_NeedsDecision == false`, abre directamente en cuadro compacto de abundancia.
* `RationViewAction RationView_Update(RationViewState* state, const Army* army, const Map* map, HexCoord pos, int sw, int sh);`
  Procesa atajos (`A` = auto, `R` = repetir, `ENTER` = confirmar), botones `+` / `-`, rueda del ratón y drag & drop. Retorna acciones tipadas para el controlador (`RATION_ACTION_SUBMIT_PLAN`, `RATION_ACTION_SLAUGHTER_CATTLE`, `RATION_ACTION_CLOSE_REPORT`).
* `void RationView_Draw(const RationViewState* state, const Army* army, const Map* map, HexCoord pos, int sw, int sh);`
  Renderiza el modal con distribución uniforme y sin solapamiento de slots o etiquetas de recursos, chips de glosario interactivos y pronóstico en tiempo real.
* `void RationView_ShowReport(RationViewState* state, const DayReport* report);`
  Despliega la página 2 con el desglose de salud, moral y bajas tras confirmar el reparto del día.

### 11.4. Mock de Desarrollo (`views/ration_mock.h`)
Implementación simulada completa para pruebas de interfaz y testing desacoplado:
* Escenarios soportados: `MOCK_SCENARIO_ABUNDANCE`, `MOCK_SCENARIO_SHORTAGE`, `MOCK_SCENARIO_STARVING_GROUP`, `MOCK_SCENARIO_ZERO_SATISFACTION`.
* Todas las consultas puras (`RationMock_GetNeeds`, `RationMock_GetStock`, `RationMock_GetGroupState`, `RationMock_Preview`, `RationMock_Validate`) se enlazan mediante `views/ration_mock.h` en modo test o simulación.

### 11.5. Extensión del HUD y Panel de Grupos (`views/hud.h`)
* **Indicador de Suministro:** Se sustituye la línea estática por días restantes individuales (`Agua`, `Vino`, `Comida`, `Carne`) y tres indicadores anímicos (`Moral`, `Salud`, `Sargentos`).
* **Modal de Grupos:** Accesible en cualquier momento pulsando la tecla `G` o haciendo clic en el botón `[GRUPOS]` de la barra de estado del ejército. Muestra el estado integral de los cuatro grupos, días sin ración y satisfacción de los Sargentos con tooltips de glosario.





