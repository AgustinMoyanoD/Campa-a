# arreglo_correcciones.md: correcciones tras el playtest

## 1. Objetivo y contexto

Juego de campaña táctica por turnos en **C puro + Raylib**, arquitectura **MVC**. Ya están implementadas las fases de `arreglo_exploracion.md` (supervivencia, noche y campamento, IA enemiga, ciudades, facciones y asedio) y el combate de `arreglo_combate.md`. Un playtest y una revisión del informe de fases encontraron bugs, problemas de legibilidad, desbalance y funciones faltantes.

Este documento tiene prioridad sobre los anteriores en lo que contradiga. Se organiza en bloques; cada bloque termina en un checkpoint.

| Bloque | Contenido |
|---|---|
| A | Bugs y desvíos respecto de la especificación |
| B | Legibilidad de la interfaz (texto muerto, tooltips, glosario) |
| C | Balance: simulador de cruce de nivel y retuneo |
| D | Descubrir asentamientos |
| E | Oficiales y motín |
| F | Sensación de la exploración |

## 2. Cómo trabajar

1. **Reproducí cada bug antes de arreglarlo** y mostrame la causa encontrada (archivo, función, línea). No arregles por intuición.
2. Por cada bloque: plan breve, implementación en pasos que compilen, commit por paso, pruebas, y **checkpoint**: esperá mi OK antes del bloque siguiente.
3. No amplíes el alcance. Lo no definido va a la sección 11.
4. Al terminar cada bloque, actualizá `DOC_API_Reference.md` y `GDD_Proyecto.md` y entregá un resumen con las decisiones tomadas ante ambigüedades.
5. **Mostrá evidencia**, no afirmaciones: salida de las pruebas, números de las simulaciones, capturas de pantalla o descripción de lo verificado en el juego.

## 3. Invariantes

- `models/` no conoce Raylib, audio, animaciones ni tiempo real.
- Todo número ajustable vive en `assets/balance.cfg` o en archivos de datos.
- Un único `DayClock`: todo cierre de franja y de día pasa por el mismo camino.
- Una sola fuente de verdad por cálculo: la vista y el HUD **consultan** al modelo, no recalculan reglas.
- Una RNG con semilla para el mundo; las simulaciones usan RNG separada.
- Las vistas devuelven acciones; el controlador las ejecuta.

## 4. Decisiones cerradas

| # | Decisión |
|---|---|
| X1 | **Forrajear** está siempre disponible y, al hacerlo, **el tiempo avanza hasta la franja de tarde**. (El informe del agente lo describió mal.) |
| X2 | Las raciones iniciales alcanzan para unos **3 días**; a partir de ahí encontrar un asentamiento es prioritario. |
| X3 | Sin comida ni agua y con el ejército exhausto, ganar peleas debe ser muy difícil. **Eso está bien**, no se toca. |
| X4 | **Motín:** el teniente queda como **líder permanente por lo menos hasta el próximo nivel**; si la moral se recuperó, **el general puede volver** (sección 9). |
| X5 | Los tenientes deben ser **visibles y útiles** para el jugador. |
| X6 | Los asentamientos deben ser **fáciles de encontrar** (carteles o indicaciones en el mapa). |

## 5. Bloque A: bugs y desvíos

### A1. Tras acampar, la mañana no deja gastar PA
Reproducí: acampar al atardecer, pasar a la mañana siguiente, intentar mover. Causas probables: una bandera de acampar que no se limpia al llegar a Dawn, o que el camino de "acampar" no ejecuta el mismo reinicio (PA sin tirar, "YA TIRASTE", contador de turno) que el de pasar turno normal.
**Arreglo:** que **ambos caminos usen la misma función de avance de reloj**. **Prueba:** después de acampar, `camping == false`, franja == Dawn, PA sin tirar, se puede tirar y mover.

### A2. El forrajeo gasta recursos en vez de dar
Reproducí un forrajeo y mostrame el `DayReport` completo. Hipótesis: (1) error de signo en el rendimiento; (2) economía: forrajear salta hasta la tarde (X1), y las franjas saltadas consumen más (~0,5 a 0,75 días) que lo que rinde la tabla actual (0,25 a 0,75 días).
**Arreglo:**
- Si hay error de signo, corregirlo.
- Recalibrá los rendimientos para que, en la tirada normal, el **neto sea positivo** (rendimiento menos el consumo de las franjas saltadas) en biomas fértiles. Valores iniciales, en días de consumo del ejército:

```ini
[FORAGE]
Enabled=true
Plains=food:1.0 water:0.5
Forest=food:1.5 water:0.5
Mountain=food:0.5 water:1.0
Desert=food:0.25 water:0.25
Water=food:0.5 water:1.5
; tirada 2d6 + Evaluacion: <6 x0.5 (y puede disparar un evento), 6-9 x1.0, >=10 x1.5
```
- **Mostrar el costo antes de elegir** (ver B): "Forrajear: ~+1,0 día, pasa hasta la tarde (−0,5 día)".
**Prueba:** en Llanura y Bosque, la tirada normal deja neto ≥ +0,25 días del recurso objetivo. Con tirada baja puede ser neto negativo (es el riesgo).

### A3. La estimación de raciones del HUD es errónea
Hoy el HUD muestra, por ejemplo, "Raciones: 1.9d [! AVISO]" desde el turno 1, y Agua 150 y Comida 130 figuran como "Escasa". Son tres cálculos distintos de lo mismo; además la estimación puede caer más de un día en un día.
**Arreglo (fuente única de verdad):**

```c
typedef struct { float campDays, marchDays, nightMarchDays; } SupplyForecast;
SupplyForecast Army_ForecastSupply(const Army* army, const Map* map, HexCoord pos);
```
Se calcula **simulando hacia adelante con la misma función real de consumo (`Army_ConsumeTurnResources`) sobre una copia del ejército**, franja por franja, hasta que una reserva llegue a 0 (tope 30 días). Escenarios: acampando, marchando de día, marchando de noche. Para el costo de bioma al marchar, usá el peor multiplicador dentro de un radio configurable (conservador) y mostralo como un rango `min – max`.
- El HUD muestra el rango (ejemplo: `1,2 – 3,0 d`).
- Los estados (OK, Escasa, Crítica) salen de **los días mínimos**, no de la cantidad absoluta: `OK ≥ 3,0`, `Escasa 1,5 a 3,0`, `Crítica < 1,5` (en `balance.cfg`). El AVISO usa `LowSupplyDays` contra el mínimo.
**Prueba:** el pronóstico coincide con el resultado de simular los días reales (misma semilla y mismas acciones) con una tolerancia de ±0,1 día.

### A4. Verificar que las correcciones del combate estén integradas
La captura del playtest muestra "[FATIGADO]" en "Asalto Frontal" en la ronda 1, texto de botones desalineado y la etiqueta "(Enemigo +2)", que son síntomas del código **anterior** a `combate_corregido/`. Confirmá si `combat.c`, `combat.h`, `combat_view.c` y `ui_widgets.c` corregidos están integrados; si no, integralos (y llamá `UI_Shutdown()` al cerrar; agregá el prototipo de `UI_ButtonClicked` a `ui_widgets.h`).
**Prueba:** en la ronda 1 ningún plan aparece fatigado; el texto de todos los botones está centrado.

### A5. Desvíos del informe de fases (corregir o justificar con evidencia)
- **Panel de acampar obligatorio:** mantenerlo, pero con **acampar como opción por defecto confirmable con ENTER**, para no agregar fricción.
- **Marcha nocturna "×1,5×1,5 PA":** confirmar que el multiplicador de costo es fijo (1,5). Lo que escala por noche consecutiva son las **penalidades de moral y salud**, no el costo de PA. Si se compone, corregirlo.
- **Facciones por `(settlementId - 1) % 3`:** reemplazar por asignación **espacial**: 3 semillas por nivel y cada cluster toma la facción de la semilla más cercana.
- **Multiplicador de botín del asedio** en `GameMaster_ResolveSiegeVictory`: moverlo al contexto de combate (`lootMultiplier`) para que lo aplique una única función de botín.
- **Costo del templo `base + count*20`:** aclarar qué es `count`. Debe ser **la cantidad de maldiciones ya quitadas** (el precio sube con cada quita), no las que se tienen.
- **IA enemiga:** entregar la lista de **comportamientos y estados que realmente existen**. La especificación pedía ALERT, Guardia, Cazador, Emboscador, Bestia, el perseguidor y los pesos de aparición nocturnos; el informe solo muestra 4 estados.
- **Pruebas de aceptación:** entregar los resultados de las 8 pruebas de la sección 13 de `arreglo_exploracion.md`.

## 6. Bloque B: legibilidad de la interfaz

El "texto muerto" se arregla con jerarquía y explicaciones a demanda, no sacando información.

### B1. Glosario y tooltips
- Archivo de datos `assets/glossary.cfg` con una entrada por término (id, título, texto breve). Ejemplo:

```ini
[FATIGADO]
Title=Plan cansado
Text=Repetir el mismo plan dos rondas seguidas reduce a la mitad sus bonos y penalizaciones.
[CHOQUE]
Title=Choque
Text=El intercambio de daño de la ronda. Un multiplicador mayor aumenta el daño recibido y el infligido.
```
- Todo término, etiqueta o estado mostrado en pantalla se dibuja con `UI_DrawChip(id)` y se explica con un **tooltip al pasar el mouse** (retardo ~0,35 s, posicionado y acotado a la pantalla), con el texto del glosario.
- `Glossary_ValidateAll()` al iniciar: **falla** si algún id usado en el código no existe en el glosario. Cubrir como mínimo: Tirada, Puntaje, Choque, Margen, Intención, Fatigado, Expuesto, Fortificado, Desorganizados, Caballería, Mando, Fuera de combate, Ventaja, Desventaja, Moral, Salud, Raciones, Infamia, Lealtad.

### B2. Tarjetas de plan (combate)
Reemplazar las líneas de texto largas por **tarjetas**: nombre, requisito de tirada (con candado si está bloqueado y cuánto falta), chips de efecto (`Tirada +2`, `Choque ×1,2`), una flecha verde o roja de ventaja o desventaja contra la intención revelada, y el chip `Cansado` si corresponde. Una línea de descripción como máximo. Las reglas siguen viniendo de `Combat_GetPlanInfo` (no se duplican en la vista).

### B3. Reglas generales de texto
- Cada texto en pantalla debe ser **accionable o explicativo**; eliminar el resto.
- Posiciones y tamaños salen de las funciones de layout y del tema (`ui_theme.h`), sin coordenadas sueltas.
- Las acciones del HUD muestran su costo antes de usarse (forrajear: tiempo y rendimiento esperado; acampar: riesgo estimado; marchar de noche: penalidades).
- Pistas de primer uso por pantalla (combate, campamento, ciudad), descartables.

**Prueba:** `Glossary_ValidateAll()` pasa; en el combate no hay ningún término sin tooltip; ningún texto se superpone en resoluciones 1280×720 y 1600×900.

## 7. Bloque C: balance y simulador de cruce

### C1. Principio
La **unidad común de balance es el "día de suministro"**: lo que consume el ejército en un día. Todo se expresa en esa unidad: raciones iniciales, rendimiento del forrajeo, botín, precios.

### C2. Simulador de cruce de nivel (`tests/sim_crossing.c`)
Sin interfaz, con RNG separada y N ≥ 1.000 niveles generados con el generador real. Un **piloto automático** recorre de A a B, y:
- se desvía al asentamiento conocido más cercano cuando `campDays` baja de 2,0 y compra lo necesario;
- acampa por defecto y marcha de noche solo si tiene salud y suministros suficientes (parametrizable);
- resuelve combates con el modelo real y una estrategia simple (mejor plan desbloqueado).

Reporta: tasa de llegada a B con vida; distribución del estado al llegar (días de suministro, salud, moral, tropas); causa de fracaso (hambre, motín, combate); días empleados; asentamientos visitados; oro gastado.

**Metas (provisorias):** un piloto competente llega a B en **≥ 60%** de los niveles, con **15 a 30%** de reservas y salud **≥ 50**.

### C3. Palancas (en `balance.cfg`)
- **Raciones iniciales:** subir hacia ~5 días, o dejar ~3 y garantizar un asentamiento a ≤ 2 días (bloque D). Elegí con el simulador y mostrame la tabla comparativa (3, 4, 5, 6 días).
- **Consumo, rendimiento del forrajeo, precios del mercado, botín** (comida y oro).
- **Botín de asedio:** debe superar el costo esperado. Valores iniciales: **4 a 6 días de suministro más 150 a 250 de oro**, sobre una pelea que cuesta ~30% del ejército.
- **Espaciado de asentamientos** (ver D1).

### C4. Retuneo de la noche y ampliación de su simulación
La simulación nocturna actual (utilidad con marcha sin cartas Δ −79,7 y salud final 4,2) no logra el objetivo de "levemente negativa". Corregir:
- **Retuneá** las penalidades para que **marchar de noche sin cartas** dé un Δ de utilidad de **−10 a −20** y termine con salud **≥ 60** a los 10 días.
- Ampliá la simulación: incluir **comida y agua** y el **perseguidor**; agregar una **política mixta** (marchar cuando hay salud y cobertura, acampar si no) y un caso con **una sola carta nocturna** (el jugador tiene 3 slots). Metas: con una carta Δ entre 0 y +10; la política mixta debe rendir al menos como la mejor de las políticas fijas.
- **Mostrá la fórmula de utilidad** (pesos incluidos) y una **tabla de sensibilidad** (±50% en cada peso) para que se vea que la conclusión no depende de pesos arbitrarios.
- Revisá los efectos reales de Artemisa, Selene y Hécate contra la especificación: la reducción de encuentros y de pérdida de salud y moral del caso con cartas parece mayor que la de las cartas descritas.

## 8. Bloque D: descubrir asentamientos

### D1. Garantía de generación
```ini
[MAPGEN]
MaxDaysToSettlement=2.0
HexPerDay=9               ; ritmo observado acampando cada noche; ajustar con el simulador
```
El generador asegura que desde **cualquier hexágono de la ruta A a B** hay un asentamiento a una distancia (por camino) ≤ `MaxDaysToSettlement × HexPerDay`. Si no se cumple, agregar Viviendas en los puntos necesarios. **Prueba:** en 1.000 niveles generados, la garantía se cumple siempre.

### D2. Postes indicadores
Estructura nueva `SIGNPOST` en cruces de camino y cada ~8 hexágonos de camino. Al **ver** uno (o pasar por él), revela el asentamiento más cercano: queda marcado en el mapa como **rumor** (ícono distinto del descubierto, con flecha y distancia en días), sin costo. Sin arte nuevo: un ícono simple. El minimapa también lo muestra.

### D3. Interfaz
Las marcas de rumor aparecen al borde de la pantalla como indicador direccional cuando están fuera de la vista.

## 9. Bloque E: oficiales y motín

### E1. Reglas definidas (X4)
- Cuando se produce un motín, **el teniente asume el mando**. El general depuesto sale del juego activo pero se conserva (`deposedGeneral`) con sus **3 cartas y sus maldiciones dormidas**.
- El teniente líder tiene **stats más bajos** y **1 slot de carta**, ocupado de inicio por la carta inherente de su **dios patrono** (según el diseño previo: dormida mientras es leal, activa si lidera). Las cartas nuevas siguen la regla de desborde (reemplazar o descartar).
- **El teniente es líder permanente por lo menos hasta el próximo nivel.** Al **inicio de cada nivel nuevo**, si el líder es un teniente y la **moral ≥ `ReturnMoral`**, se dispara el evento `ev_general_regresa` con dos opciones: (1) **reponer al general** (recupera sus cartas y stats; el teniente vuelve a ser oficial con lealtad reducida), o (2) **mantener al teniente** (el general se pierde definitivamente). Si la moral es menor, el teniente sigue y se reevalúa en el nivel siguiente.
- Tras el motín: moral fija en `MoralAfterMutiny` y un período de gracia `GraceDays` sin nuevo chequeo (evita bucles). Otro motín contra el teniente líder es posible con otro teniente; si no quedan tenientes, ocurre una **deserción masiva** en lugar de motín.

### E2. Disparo del motín (telegrafiado)
`Army_CalculateMutinyRisk` y `Army_CheckMutiny` existen pero no se llaman. Llamarlos al cerrar el día (`Army_EndDay`), con avisos previos:

```ini
[MUTINY]
WarnMoral1=35         ; evento de rumores
WarnMoral2=15         ; ultimatum con opciones
CheckMoral=10         ; desde aqui se chequea el motin cada dia
MoralAfterMutiny=35
GraceDays=3
ReturnMoral=60
```
- Rumores (≤ `WarnMoral1`) y ultimátum (≤ `WarnMoral2`) como **eventos del Event Engine** con opciones (raciones dobles, ejecutar al cabecilla, prometer un saqueo).
- Desde `CheckMoral`, cada día el motín se sortea con el riesgo existente (moral y lealtad). **No es solo suerte**: depende de lealtad, que el jugador puede mover.

### E3. Visibilidad y utilidad de los tenientes
- **Panel de oficiales** (tecla `O` y acceso desde el HUD): por oficial, retrato (placeholder), nombre, rol, stats, **barra de lealtad**, dios patrono y su carta dormida; el líder marcado.
- **Roles con efecto real** (datos en `.mod`, aplicados mientras el oficial está en el ejército):

| Rol | Efecto inicial |
|---|---|
| Explorador | Al comenzar el día revela asentamientos en un radio de 8 hexágonos |
| Intendente | +25% al rendimiento del forrajeo |
| Capellán | +2 de moral por día mientras la moral está bajo 50 |

Hooks y efectos nuevos a agregar si faltan: `HOOK_DAY_START`, `HOOK_FORAGE_YIELD`, `reveal_settlements`, `forage_yield`, `moral_daily`.
- **Consejos:** eventos (`Trigger=ADVICE`) en momentos clave (decisión de acampar, hambre o sed desde el día 2, antes de una pelea peligrosa) donde un teniente propone una opción. Aceptar o ignorar mueve su **lealtad** (efecto nuevo `loyalty:<rol>:<delta>`). Escribir al menos **6 consejos**.

```ini
[LOYALTY]
AcceptAdvice=+3
IgnoreAdvice=-2
DailyDriftWhenMoralBelow30=-1
```

**Pruebas:** con moral 10, durante varios días el motín ocurre con la frecuencia esperada (muestreo); tras el motín el líder activo es el teniente y los modificadores activos se recalculan (el general queda dormido); el evento de regreso aparece solo al inicio de un nivel con la moral suficiente; ambas opciones restauran un estado consistente; no hay motines durante `GraceDays`.

## 10. Bloque F: sensación de la exploración

La exploración sigue sintiéndose tosca. Hay una capa de presentación (`tween`, `sequence`, `ui_widgets`, `audio_manager`); usala. **Primero verificá qué de esto ya existe** y reportalo.

- **Clic en destino con pathfinding (A\*)** sobre coordenadas axiales con los costos de bioma, solo por casillas conocidas. **Vista previa** de la ruta con su costo: `7 PA | −0,3 d` (PA y días de suministro). El camino se detiene ante un evento o un enemigo.
- **Movimiento interpolado** del token. **Sacar del `Army`** los campos de animación (`movePath`, `moveTimer`, `isMoving`, `currentPathIndex`): son presentación dentro del modelo; la posición lógica queda separada de la visual.
- **Niebla de guerra** con fundido al revelarse; **transición de tinte** entre las franjas del día con tweens.
- **Audio:** pasos (con variación por bioma), ambiente por bioma y por hora. Placeholders generados (por ejemplo con jsfxr o bfxr); el módulo de audio debe tolerar archivos faltantes.
- **Cámara** que sigue al ejército con suavizado.

**Prueba:** se puede mover a cualquier casilla conocida con un clic; la vista previa coincide con el costo real; el movimiento se interrumpe ante enemigos; el modelo `Army` ya no contiene estado de animación.

## 11. Fuera de alcance

- Nuevos sistemas no descritos (comercio entre facciones, nuevos tipos de enemigo, árbol de habilidades de oficiales).
- Arte final, música y efectos avanzados (solo placeholders).
- Guardado y carga de partidas.
- Estado "herido" con recuperación en el tiempo.
- Muerte o captura de oficiales fuera del motín.

## 12. Entregables

1. Por bloque: causas encontradas, código con commits por paso, y evidencia (salidas de pruebas y simulaciones).
2. `assets/balance.cfg` y `assets/glossary.cfg` actualizados.
3. Datos nuevos: eventos de rumores, ultimátum, regreso del general y consejos; roles de oficiales; postes indicadores.
4. `tests/sim_crossing.c` y la simulación nocturna ampliada, con tablas de resultados y sensibilidad.
5. Lista de comportamientos y estados de IA enemiga existentes.
6. Resultados de las pruebas de aceptación de `arreglo_exploracion.md`.
7. `DOC_API_Reference.md` y `GDD_Proyecto.md` actualizados.
8. Resumen final con decisiones ante ambigüedades y lo que no pudiste completar.
