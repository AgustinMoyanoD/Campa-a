# arreglo_exploracion.md: supervivencia, noche, enemigos y ciudades

## 1. Objetivo y contexto

Trabajás sobre un juego de campaña táctica por turnos en **C puro + Raylib** con arquitectura **MVC** (`src/models/`, `src/views/`, `src/controllers/`). La run son 4 niveles (cada uno con un bioma), de un punto A a un punto B, con agotamiento de recursos y dificultad creciente.

Hoy la exploración no tiene presión ni propósito:

1. **No hay penalidades** por quedarse sin comida o agua.
2. **La noche** casi no cambia nada (solo baja el radio de la niebla y habilita eventos raros).
3. **Las ciudades** no hacen nada.
4. **Los enemigos** casi no se mueven solos: es fácil rodearlos y escapar.

Este documento define los cambios. Tiene prioridad sobre `GDD_Proyecto.md` en estos temas. El combate está definido en `arreglo_combate.md`; acá solo se tocan los puntos de contacto con él (sección 11).

## 2. Cómo trabajar

1. **Auditoría primero** (sin modificar código). Leé `army.h/.c`, `game_master.h/.c`, `map_generator`, `hex_tile`, `event.h/.c`, `effect.h/.c`, `enemy.h/.c`, `modifier.h/.c`, `general.h/.c`, `hud`, `map_view`, `enemy_view` y los datos de eventos y modificadores. Entregame un informe con:
   - cómo funciona hoy el **ciclo de día** (franjas, qué avanza el tiempo, dónde se chequea la hora) y **cada cuánto se consume** comida y agua (`Army_ConsumeTurnResources`);
   - qué es un **turno del jugador** (bloque de PA) y cuántos por día;
   - qué **estructuras** existen y qué datos tienen;
   - cómo se **mueven y detectan** hoy los enemigos;
   - qué efectos y condiciones de eventos existen (`cattle`, `salud`, `Requires`, `MinCattle`...);
   - de dónde sale hoy el **oro**.
   **Detenete y esperá mi OK.**
2. Con el OK, mostrame un plan por pasos y esperá confirmación.
3. Implementá por fases (sección 12), en pasos que compilen, con commit por paso y **checkpoint al final de cada fase**. Esperá mi OK antes de la siguiente.
4. No amplíes el alcance. Lo no definido va a la sección 13.
5. Al terminar cada fase: pruebas y simulaciones sin interfaz pasando, `DOC_API_Reference.md` y `GDD_Proyecto.md` actualizados, y un resumen con las decisiones que tomaste ante ambigüedades.

## 3. Invariantes (reglas duras)

- **I1.** `models/` no conoce Raylib, audio, animaciones ni tiempo real.
- **I2.** Todo número ajustable vive en `assets/balance.cfg` o en archivos de datos. Cero números mágicos de balance.
- **I3.** Contenido en datos: tablas de hambre, precios, campamento, comportamiento de enemigos, eventos y modificadores se cargan de archivos (reutilizá el lector INI genérico y `effect.c`).
- **I4.** Un **único módulo de reloj** (`DayClock`) emite los eventos "cambió la franja" y "terminó el día". Nada más chequea la hora por su cuenta.
- **I5.** Toda aleatoriedad del mundo sale de **una RNG con semilla** (no `rand()` ni `GetRandomValue` en modelos). Las simulaciones usan RNG separada.
- **I6.** Las vistas no modifican el modelo: devuelven la acción del jugador y el controlador la ejecuta.
- **I7.** Todo lo que pasa al cerrar el día queda en un `DayReport` (qué se consumió, desertores, muertos, cambios de moral y salud) para que la interfaz lo muestre. Nada ocurre "en silencio".
- **I8.** Ninguna regla de balance se duplica en la vista: la vista consulta funciones del modelo.

## 4. Decisiones cerradas (no las reabras)

| # | Decisión |
|---|---|
| E1 | Quedarse sin comida o agua tiene una **escalera de consecuencias**: un día de gracia, al segundo baja la moral, al tercero hay desgaste físico (salud baja, desertan o mueren soldados). |
| E2 | Con ganado disponible se habilita un **evento para sacrificarlo** a cambio de comida. |
| E3 | Hay **eventos con un dios** que ofrece ayuda a cambio de un peso: una maldición a cambio de comida o agua. |
| E4 | Los mercados son **mercados reales**: compra y venta de recursos. |
| N1 | Al terminar la tarde se elige **acampar** (no se puede seguir marchando, riesgo de emboscada con una tirada ligada a estadísticas) o **seguir marchando de noche**. |
| N2 | La noche está cargada de **eventos peligrosos** (bestias, emboscadas), **mayor costo de movilidad**, y **baja de moral y salud** en un ejército que no descansa. |
| N3 | Debe haber **equilibrio**: a veces conviene seguir marchando. Hay **cartas** que mejoran el desempeño nocturno; de noche hay **menos tropas grandes** y **más bandidos y tropas de bajo nivel**. |
| C1 | **Armería:** algunos Iniciados suben a Soldado (costoso) y algunos Soldados a Sargento (más costoso). **Mercado:** intercambio de monedas por recursos. |
| C2 | Cada **cluster de ciudad pertenece a una facción**. Cada facción tiene una **opinión** del ejército, función de la infamia y del trato recibido. |
| C3 | Al llegar al radio de una ciudad se ofrece **asediarla**, lo que dispara la **alerta de la guardia local**. |
| M1 | Los **enemigos** deben moverse por su cuenta, patrullar, detectar y perseguir, de modo que no sea trivial rodearlos y escapar. |

## 5. Reglas con valor por defecto (provisorias, configurables)

No están confirmadas. Implementalas con estos valores y expuestas en `balance.cfg`; las marcadas **(propuesta)** se pueden desactivar con `Enabled=false`:

- **Cadencia de consumo:** respetá la que ya existe. Los **días sin suministro** se cuentan al **cerrar el día** (después de la Noche o al acampar).
- **Contador de días:** si al cerrar el día la reserva está en 0, el contador de ese recurso sube 1; si hubo suministro, baja 1 (recuperación gradual, sin vaivén).
- **El agua castiga más fuerte que la comida** en la misma escalera (mismos días, números más duros).
- **Forrajeo (propuesta):** acción que usa un turno y recupera algo de comida y agua según el bioma, como salida de emergencia que no depende del oro.
- **Pacto divino acotado:** una vez por nivel, y solo si hay un slot de maldición libre (el límite actual es 4).
- **Mercado como tienda** con stock limitado y precios, no como simulación de comercio.
- **Perseguidor (propuesta):** un escuadrón que aparece detrás del jugador cada cierto número de días y lo rastrea, para que acampar tenga costo.
- **Zona de control y ventaja de huida (propuesta):** ver sección 8.

## 6. Supervivencia: comida y agua

### 6.1 Modelo

Agregá a `Army`: `daysWithoutFood`, `daysWithoutWater` (int). Funciones nuevas en el modelo:

```c
void Army_EndDay(Army* army, DayReport* report, Rng* rng);   /* actualiza contadores y aplica la escalera */
float Army_DaysOfSupply(const Army* army);                   /* min(comida, agua) en dias de consumo actual */
```

`Army_EndDay` aplica, por cada recurso en 0, la fila de la tabla según el contador. Desertores y muertos se sortean por tropa (Iniciados primero) con multiplicadores por rango. Todo queda en el `DayReport`.

### 6.2 Tablas (`balance.cfg`)

```ini
[STARVATION_FOOD]
; Dia = dias consecutivos con la reserva en 0. La ultima fila aplica de ahi en adelante.
Day1=moral:0   salud:0   desert:0 death:0
Day2=moral:-6  salud:0   desert:0 death:0
Day3=moral:-8  salud:-10 desert:3 death:2

[STARVATION_WATER]
Day1=moral:0   salud:0   desert:0 death:0
Day2=moral:-8  salud:-5  desert:0 death:0
Day3=moral:-10 salud:-18 desert:4 death:3

[STARVATION_RANK_MULT]
Iniciados=1.0
Soldados=0.5
Sargentos=0.25

[SUPPLY_WARNING]
LowSupplyDays=2.0
```

Las filas de moral y salud usan la escala actual de `Army`. `desert` y `death` son porcentajes por tropa.

### 6.3 Interfaz y datos

- **HUD:** mostrar "raciones para N días" (con `Army_DaysOfSupply`) y un indicador de estado de cada recurso (OK, escasa, agotada día 1/2/3). Aviso cuando `Army_DaysOfSupply` baja de `LowSupplyDays`.
- **Resumen de fin de día:** panel breve con el `DayReport`.
- **Efectos nuevos** en `effect.c`: `cattle`, `salud` (y los que falten de `food`, `water`, `moral`). Condiciones para eventos y opciones: `MinCattle`, `MaxFoodDays`, `DaysWithoutFood`, `DaysWithoutWater`, y un `Requires=` por opción (por ejemplo `Requires=cattle>=1`) si todavía no existe, con la opción deshabilitada y su motivo si no se cumple.

### 6.4 Eventos de supervivencia (reutilizá el Event Engine)

1. **Sacrificar ganado** (E2): condiciones `MinCattle=1` y poca comida; opciones: sacrificar N cabezas (`cattle:-N`, `food:+X`) o negarse. Mencionar en el texto que perder el ganado reduce el amortiguador de comida.
2. **Pacto divino** (E3): condiciones de hambre o sed a partir del día 2 y slot de maldición libre; límite una vez por nivel. Aceptar: `food:+X` o `water:+X` y `addmod:<maldicion>`; rechazar: nada. Crear **dos maldiciones de ejemplo** con tradeoff y salida (usá el formato `.mod` existente): una que sube el consumo de agua durante varios días y otra que reduce la visión, ambas removibles en un templo (fase 4).
3. **Murmullos** (opcional): al día 3 de hambre, evento con decisión (ejecutar al cabecilla: sube infamia, baja moral ahora; o doblar raciones: empeora la reserva).

### 6.5 Forrajeo (propuesta)

Acción disponible fuera de combate que consume el turno. Rinde según el bioma y una tirada `2d6 + Evaluación`:

```ini
[FORAGE]
Enabled=true
; rendimiento en "dias de consumo del ejercito actual"
Plains=food:0.5 water:0.25
Forest=food:0.75 water:0.25
Mountain=food:0.25 water:0.5
Desert=food:0.1 water:0.1
Water=food:0.25 water:1.0
; tirada: <6 rinde x0.5 y puede disparar un evento; 6-9 x1.0; >=10 x1.5
```

### 6.6 Botín (prerrequisito de la economía)

Hoy `lootGold` y `lootFood` del combate nunca se asignan, así que ganar no da nada. Implementá `Combat_ComputeLoot` (en el commit de victoria): oro y comida proporcionales a la fuerza base del enemigo, con una variación, configurable (`[LOOT]`). Sin oro no hay economía.

## 7. Noche y campamento

### 7.1 El reloj (I4)

Si el ciclo de día está disperso, reducilo a un módulo `DayClock` con franjas (Mañana, Mediodía, Tarde, Noche), y dos señales: `SlotChanged` y `DayEnded`. El cierre del día llama a `Army_EndDay`.

### 7.2 La decisión de la tarde

Al terminar la Tarde se muestra un **panel pequeño** con dos opciones: **Acampar** (por defecto, con una tecla de confirmación) o **Seguir marchando**. El panel muestra: cobertura del terreno, riesgo estimado (Bajo/Medio/Alto), y los días de suministro que quedan. No debe agregar fricción: acampar es el camino rápido.

### 7.3 Acampar

- No se puede mover. El consumo de la noche se aplica igual.
- **Recupera** moral y salud (tabla), pero no recupera si el ejército está en hambre o sed a partir del día 2.
- **Tirada de emboscada** (no es pura suerte, depende de stats): `2d6 + Evaluación + cobertura del terreno` contra un objetivo. Si falla, se dispara un **evento de emboscada** (`Trigger=CAMP_AMBUSH`) en el que se puede pagar para evitarla o pelear, y si se pelea el combate empieza con la bandera `ambushed` (sección 11).
- **Cobertura por terreno** (tabla): bosque da cobertura, llanura expone, el desierto penaliza; cerca de una ciudad aliada no hay emboscada. Esto hace que **dónde acampar** sea una decisión de ruta al final de la tarde.

```ini
[CAMP]
Recover_Moral=6
Recover_Salud=5
AmbushTarget=7
StarvingNoRecovery=true
Cover_Forest=2
Cover_Hills=1
Cover_Plains=0
Cover_Mountain=1
Cover_Desert=-1
Cover_Ruins=1
```

### 7.4 Seguir marchando de noche

```ini
[NIGHT_MARCH]
MoveCostMult=1.5            ; el costo de movilidad se redondea hacia arriba
Moral_PerTurn=-2
Salud_PerTurn=-2
ConsecutiveMult=0.5         ; cada noche consecutiva sin acampar suma +50% a las penalidades
EncounterChance=35          ; porcentaje por turno nocturno
```

- El radio de la niebla ya se reduce de noche (mantener).
- Agregá a `Army` el contador `consecutiveNightMarches` (se reinicia al acampar).
- Cada turno nocturno se tira `EncounterChance`: si sale, se elige un evento o un encuentro del **pool nocturno**. Antes la frecuencia dependía de filtros y probabilidades bajas: ahora está garantizada.

### 7.5 Enemigos nocturnos

Las plantillas enemigas tienen **pesos de aparición por franja**. De noche: menos patrullas grandes y disciplinadas (`NIGHT` peso 0.3) y más bandidos y bestias (peso 2.0). Las bestias son un tipo nuevo de enemigo (sin botín de oro ni infamia).

### 7.6 Cartas y hooks nocturnos

Agregá a `modifier.c` los hooks que falten: `HOOK_NIGHT_PENALTY`, `HOOK_AMBUSH_ROLL`, `HOOK_NIGHT_ENCOUNTER_CHANCE` (y confirmá que `HOOK_MAP_VISION` y el de costo de movilidad existen). Creá **3 cartas de ejemplo** con tema de dioses de la noche, en formato `.mod`:

| Carta | Efecto |
|---|---|
| Artemisa | La niebla no se reduce de noche |
| Selene | −1 al costo de movilidad nocturno |
| Hécate | Mejores resultados en eventos nocturnos, a cambio de infamia |

### 7.7 Eventos nocturnos

Escribí al menos **8 eventos** en el formato existente (`Trigger=NIGHT` o `CAMP_AMBUSH`, con `Time=NIGHT`), con texto breve y provisorio (lo reescribe el desarrollador): ataque de lobos, bandidos en el camino, centinela dormido, deserción nocturna, tormenta, luces en el bosque, un enemigo avistado desde lejos, y una emboscada al campamento. Todos con una opción que dependa de una estadística, no de pura suerte.

### 7.8 Equilibrio

Hacé una **simulación sin interfaz** que compare las dos políticas (acampar siempre vs marchar de noche) y reporte comida consumida, progreso, bajas y salud. Meta: para un ejército promedio marchar de noche tiene valor esperado **levemente negativo**, y con cartas nocturnas es **positivo**. Así a veces conviene seguir.

## 8. Enemigos con IA

### 8.1 Problema

Hoy el combate solo ocurre cuando el jugador pisa al enemigo, y los enemigos casi no se mueven, así que se los rodea y se escapa sin costo.

### 8.2 Modelo (`models/enemy.c` o `enemy_ai.c`, sin Raylib)

Ampliá `EnemySquad` (reutilizá lo existente; no dupliques la estructura):

```c
typedef enum { AI_PATROL, AI_GUARD, AI_HUNTER, AI_AMBUSH, AI_BEAST } EnemyBehavior;
typedef enum { AIS_IDLE, AIS_PATROLLING, AIS_ALERT, AIS_CHASING,
               AIS_SEARCHING, AIS_RETURNING, AIS_RESTING } EnemyAIState;
/* campos nuevos: behavior, aiState, mobility, senseRadius, leash, homeHex,
   routeIndex/route[], lastKnownPlayerPos, chaseTurnsLeft, skipNextPhase */
```

Los comportamientos y valores por defecto vienen de **datos** (reutilizá el formato de plantillas enemigas existente; si no hay, creá un `.enm` en formato INI con el lector genérico):

| Comportamiento | Qué hace |
|---|---|
| Patrulla | Recorre una ruta por caminos entre puntos (estructuras), y persigue si detecta |
| Guardia | Se queda cerca de un punto, con un radio máximo de alejamiento |
| Cazador | Rastrea activamente al jugador |
| Emboscador | Se esconde cerca de caminos y ataca al pasar a ≤ 2 hexágonos |
| Bestia | Solo de noche, rápida, sin botín |

### 8.3 Fase de enemigos

**Después de cada turno del jugador** (un bloque de PA) corre una fase de enemigos: cada escuadrón activo gasta su `mobility` en PA con los mismos costos de terreno que el jugador, siguiendo su máquina de estados:

- **Detección:** si el jugador está dentro de `senseRadius` (día y noche con valores distintos, y mayor para ejércitos grandes), pasa a ALERT y luego CHASING.
- **Persecución:** camina hacia la última posición conocida (A\* o BFS sobre la grilla hexagonal; reutilizá el pathfinding de exploración si existe). Si pierde al jugador `chaseTurnsLeft` turnos, pasa a SEARCHING y luego RETURNING a su base.
- **Contacto:** si queda a distancia ≤ 1 del jugador (o el jugador lo pisa), se inicia combate. **El iniciador queda registrado** (sección 11).
- **Rendimiento:** solo se simulan los enemigos dentro de `ActivityRadius` del jugador.

```ini
[ENEMY_AI]
ActivityRadius=25
SenseRadius_Day=3
SenseRadius_Night=2
SenseRadius_ArmySizeBonus=1
ChaseTurns=6
ZOC_Enabled=true          ; (propuesta) salir de un hexagono adyacente a un enemigo cuesta +1 PA
ZOC_ExtraCost=1
FleeHeadStartPhases=1     ; (propuesta) tras huir, el enemigo salta su proxima fase
```

### 8.4 Después de huir

Tras `COMBAT_END_FLEE`, `STALEMATE` o `DEFEAT`, el jugador vuelve a su posición previa y el enemigo entra en CHASING durante `ChaseTurns`. Para que no haya combates encadenados injustos, el enemigo **salta su siguiente fase** (`FleeHeadStartPhases`). Así huir tiene costo y no es un escape garantizado.

### 8.5 Perseguidor (propuesta)

```ini
[HUNTER]
Enabled=true
SpawnEveryDays=4
MobilityMult=0.9          ; levemente mas lento que el ejercito promedio
```

Un escuadrón Cazador aparece en la retaguardia (cerca del punto A o del último campamento) cada `SpawnEveryDays` días y rastrea al jugador. Esto conecta el reloj: acampar y forrajear le dan tiempo para acercarse. Es desactivable.

### 8.6 Legibilidad

El jugador debe poder leer a los enemigos: ícono de estado sobre cada escuadrón visible (calma, alerta, persiguiendo), y rango de amenaza al pasar el mouse. Movimiento interpolado en `enemy_view` (la posición visual separada de la lógica). Los enemigos siguen ocultos por la niebla fuera de la visión.

## 9. Ciudades

### 9.1 Estructuras y menú

Tipos y funciones mínimas:

| Tipo | Funciones |
|---|---|
| Vivienda | Comprar comida y agua en poca cantidad, descansar |
| Castillo | Armería, guarnición, objetivo de asedio |
| Centro urbano | Mercado, armería, hospital, templo |

**Al pasar por el radio** de una estructura se muestra un aviso no modal ("Interactuar"); el menú solo se abre si el jugador lo elige, para no interrumpir el paso. Opciones según tipo y opinión: Comerciar, Reclutar, Entrenar, Descansar, Curar o purificar, Asediar, Salir.

### 9.2 Mercado (E4, C1)

Una tienda con **stock limitado por visita** (generado con la RNG con semilla y la posición de la estructura, para que sea consistente al volver) y precios:

```ini
[MARKET]
SellRatio=0.4              ; el precio de venta siempre es menor que el de compra
PriceFood=2
PriceWater=3
PriceCattle=25
PriceInitiate=5
; stock por visita segun el tipo de estructura
StockVivienda=food:20 water:20
StockUrbano=food:100 water:100 cattle:6 initiates:20
```

Las unidades y precios son provisorios (deben coincidir con las unidades de `Army`). Precio final = base × modificador por opinión de la facción (9.4). Compra y venta de comida, agua, ganado; reclutar Iniciados.

### 9.3 Armería, hospital y templo

```ini
[ARMORY]
PromoteInitiateCost=15
PromoteInitiateMaxPerVisit=10
PromoteSoldierCost=60
PromoteSoldierMaxPerVisit=2
SergeantPerSoldiers=15     ; tope: Sargentos <= Soldados / 15
TrainingTakesDay=true      ; entrenar consume el dia (y sus raciones)

[SERVICES]
HealGoldPerSalud=2
TempleCurseRemoveCost=40
InnRestCost=10
```

- **Ascensos:** Iniciado a Soldado (costoso) y Soldado a Sargento (más costoso), con topes por visita y el tope de Sargentos. Conservación de tropas: `iniciados − n`, `soldados + n`.
- **Hospital:** cura salud por oro. Esto resuelve qué recupera la salud persistente fuera del combate.
- **Templo:** quita una maldición por oro (el costo sube con cada quita).
- **Posada:** descansa recuperando moral, consumiendo el día.

### 9.3.1 Orden de implementación de las ciudades

Las secciones 9.1 a 9.3 se implementan primero **sin facciones**: la opinión es fija en Neutral y los modificadores de precio valen 1.0.

### 9.4 Facciones (C2)

Cada cluster de ciudad pertenece a una **facción** (3 por nivel, asignadas por zona del mapa al generarlo).

```c
typedef struct { int id; char name[32]; int temperament; /* -1 odia la infamia, +1 la respeta */ int opinion; /* -100..100 */ } Faction;
```

- **Opinión inicial:** `baseline + temperament * (infamia - 50) * k`, de modo que cada facción valora la infamia distinto.
- **Se mueve** con efectos de eventos (`opinion:<facción>:<delta>`), con acciones (asediar una ciudad de la facción) y con condiciones de eventos (`MinOpinion`, `MaxOpinion`).
- **Niveles de opinión** (umbrales en `balance.cfg`): Hostil (puertas cerradas, la guardia ataca), Desconfiado (precios ×1.5, servicios limitados), Neutral, Amistoso (precios ×0.85), Aliado (precios ×0.7 y descanso gratis).

### 9.5 Asedio (C3)

Opción del menú en Castillos y Centros urbanos. Al **elegirla** (proponer el asedio) se dispara la **alerta de la guardia local**.

- **Alerta:** 0 calma, 1 alerta (puertas cerradas: sin servicios; aparece una patrulla de la guardia que persigue), 2 asedio.
- **Combate:** reutiliza el motor de combate con un enemigo de guarnición: etiqueta nueva `FORTIFIED`, más salud y más rondas (parametrizá `MaxRounds` y la salud inicial del enemigo en el contexto de combate).
- **Resultado:** victoria da botín alto, sube infamia, baja la opinión de la facción y alerta a los demás clusters de esa facción por unos días; la estructura queda ocupada o arruinada. Derrota: flujo normal de derrota.

```ini
[SIEGE]
MaxRounds=6
GarrisonHealth=150
OpinionLossOnSiege=40
InfamyGain=15
AlertDays=5
LootMultiplier=3.0
```

## 10. Interfaz mínima por fase

Usá el UI kit existente (`ui_widgets`, `ui_theme`). Sin arte nuevo: lo necesario para jugar y probar.

| Fase | Interfaz |
|---|---|
| 1 | Indicadores de suministro y "raciones para N días" en el HUD, panel de fin de día, botón de forrajeo |
| 2 | Panel de la tarde (acampar o seguir), indicador de noche, panel de emboscada |
| 3 | Íconos de estado y rango de amenaza de enemigos, movimiento interpolado |
| 4 | Menú de estructura, panel de mercado, panel de armería, curar y purificar |
| 5 | Indicador de opinión de la facción, panel de asedio con estimación |

## 11. Puntos de contacto con el combate (`combat.c`)

Cambios mínimos, el resto del combate no se toca:

- **`ambushed`:** campo nuevo en el contexto. Si es verdadero, el combate **salta la Evaluación** (información 0 forzada, intención oculta) y la primera ronda tiene −2 al puntaje de planificación.
- **`initiator`:** quién inició el contacto (jugador o enemigo); queda registrado. Solo `ambushed` cambia reglas en esta versión.
- **Parámetros del combate:** `maxRounds` y la salud inicial del enemigo pasan de constantes a campos del contexto (para el asedio).
- **Etiqueta `FORTIFIED`** en `enemy.h`.
- **Botín:** `Combat_ComputeLoot` (sección 6.6).
- **Después de huir:** el controlador pone al enemigo en CHASING (sección 8.4).

## 12. Fases (con checkpoint)

| Fase | Contenido | Prioridad |
|---|---|---|
| 0 | Auditoría (sección 2) | obligatoria |
| 1 | `DayClock`, supervivencia (6.1 a 6.6), pacto divino, botín | alta |
| 2 | Campamento y noche (7.1 a 7.8), contacto `ambushed` | alta |
| 3 | Enemigos con IA (8.1 a 8.6) | alta |
| 4 | Ciudades sin facciones: menú, mercado, armería, hospital, templo | media |
| 5 | Facciones y asedio (9.4, 9.5) | baja, se puede dejar para después del MVP |
| 6 | Pasada de contenido y equilibrio con simulaciones | al final |

La fase 3 puede ir en paralelo con la 2. Las fases 4 y 5 dependen del oro de la fase 1.

## 13. Pruebas de aceptación (arnés sin interfaz)

1. **Hambre:** con la comida en 0, simular 5 días y verificar contadores y efectos contra la tabla; verificar la recuperación gradual al volver a haber suministro.
2. **Reloj:** exactamente un `DayEnded` cada 4 franjas; el cierre del día se ejecuta una sola vez.
3. **Conservación:** ningún recurso queda negativo; las tropas antes = tropas después + desertores + muertos.
4. **Campamento:** no hay movimiento; el consumo continúa; la probabilidad de emboscada crece cuando bajan la cobertura o la Evaluación (muestreo de 10.000 tiradas).
5. **Noche:** el costo de movimiento nocturno aplica el multiplicador y las penalidades por turno escalan con las noches consecutivas.
6. **Mercado y armería:** compra y venta conservan oro y recursos; el precio de venta es siempre menor que el de compra; el stock baja; los ascensos respetan costos, topes y conservación de tropas.
7. **Enemigos:** determinismo con la misma semilla; nunca entran a hexágonos intransitables; la velocidad de persecución no supera `mobility`; la persecución termina tras `ChaseTurns`; el contacto dispara un solo combate; `ZOC` aumenta el costo de salir; tras huir el enemigo salta una fase.
8. **Asedio y facciones** (fase 5): la alerta sube al proponer el asedio; la opinión baja; el botín se acredita.

## 14. Fuera de alcance (no lo implementes)

- Diplomacia entre facciones, guerras entre facciones, rutas comerciales.
- Arte, audio y efectos visuales nuevos (iluminación nocturna, partículas).
- Cambios en el motín y la traición de oficiales.
- Estado "herido" con recuperación en el tiempo.
- Guardado y carga de partidas.
- Cierre del balance final (solo valores iniciales y simuladores).

## 15. Entregables

1. Informe de auditoría.
2. Código por fase, con commits por paso.
3. `assets/balance.cfg` con todas las secciones nuevas.
4. Datos nuevos: eventos de supervivencia y nocturnos, maldiciones y cartas de ejemplo, plantillas enemigas, tablas de mercado.
5. Arnés de pruebas y simulaciones (sección 13), incluida la comparación de políticas nocturnas.
6. `DOC_API_Reference.md` y `GDD_Proyecto.md` actualizados.
7. Resumen final con las decisiones tomadas ante ambigüedades y lo que no pudiste completar.
