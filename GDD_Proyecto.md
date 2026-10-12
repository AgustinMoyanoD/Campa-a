# Documento de Diseño del Proyecto (GDD) - Motor Táctico MVC en C

## 1. Visión General

El proyecto es un motor de juego de estrategia y rol táctico sobre mapa hexagonal, construido puramente en C y utilizando la librería Raylib para el renderizado. Su enfoque es la simulación de logística militar, eventos narrativos dinámicos, y combates de batallones gobernados por tiradas de dados 2d6 y un robusto sistema de Modificadores, Cartas Divinas y Maldiciones.

***

## 2. Arquitectura de Software (Patrón MVC)

El código base está estrictamente organizado siguiendo un patrón **MVC (Modelo-Vista-Controlador)**. Esto evita el "código espagueti" en el bucle principal (main.c) y permite que el proyecto crezca sin romperse.

* **src/models/**: La lógica pura, matemática y reglas de negocio. No saben nada sobre cómo se dibujan las cosas en pantalla. (Ej: army.c, hex\_tile.c, event.c, modifier.c).

* **src/views/**: Encargada estrictamente de dibujar en pantalla usando Raylib y escuchar el mouse/teclado. (Ej: map\_view\.c, event\_view\.c, hud.c, combat\_view\.c).

* **src/controllers/**: El "puente" que conecta modelos y vistas. Toman decisiones basadas en las reglas. (Ej: game\_master.c, camera\_controller.c).

***

## 3. Análisis Profundo de los Sistemas Principales

### 3.1. Generación de Mundo y Terreno (MapGenerator)

El mapa no es estático; se genera mediante algoritmos procedurales asegurando que cada campaña sea única.

* **Geometría de Coordenadas Axiales:** Se usan ejes (Q, R) en lugar del tradicional (X, Y). Esto permite calcular distancias reales en forma de panal de abejas, determinando qué hexágonos son adyacentes de manera matemáticamente perfecta.

* **Distribución de Biomas:** El motor inyecta terrenos (Llanuras, Bosques, Montañas, Agua). Cada bioma expone un bloque de propiedades:

  * *Costo de PA:* Atravesar montañas cuesta más puntos de acción que un sendero.

  * *Consumo Climático:* Zonas desérticas o montañosas queman tus reservas de agua y comida el doble de rápido.

### 3.2. Renderizado Isométrico y Visión (MapView & FoW)

* **Perspectiva Isométrica Real:** Todo el mundo es renderizado usando una escala de profundidad Y \* 0.5, logrando que la cuadrícula se acueste en un pseudo-3D. Los brillos de interacción se calculan con esta misma trigonometría.

* **Niebla de Guerra de 3 Capas (FoW):**

  1. *Desconocido:* Fuera del rango, no se dibuja (Dynamic Culling).
  2. *Visitado (Memoria):* El terreno queda en un registro persistente ampliado. Se dibuja oscurecido; sabes que hay una montaña, pero no si hay un enemigo en ella.
  3. *Visible (Línea de Visión):* Hexágonos iluminados a todo color alrededor del jugador, procesados con Raycasting y obstaculización visual.

### 3.3. El Orquestador del Juego (Game Master)

Es la "mesa de rol" virtual. Administra los límites del jugador.

* **Flujo de Puntos de Acción (PA):** Inicias en 0. Para poder moverte, debes tirar físicamente los dados (consumiendo la interacción del turno). El total de tus PA será: Base (ej: 3) + Cara del Dado 1 + Cara del Dado 2 + Modificadores Activos.

* **Penalidades Nocturnas:** Al llegar la Noche, la visibilidad de tu ejército se recorta, provocando que la oscuridad te encierre, aumentando el peligro de ser emboscado si decides seguir marchando sin descansar.

* **Sistema de Infamia:** Escala de 0 a 100%. Representa el odio o el miedo que el mundo te tiene. Se altera tomando decisiones amorales en los eventos, afectando la probabilidad futura de hostilidad.

### 3.4. Motor de Modificadores (ModifierRegistry & Modifier\_Query)

El núcleo de personalización del ejército (Cartas, Eventos, Buffs, Artefactos).

* **Arquitectura de Consultas (Query-Driven):** Los stats no se mutan en duro. Cada vez que se requiere una estadística (Daño, Puntos de Acción, Visión), el juego invoca a Modifier\_Query, pasando un contexto.

* **Archivos .mod Compilados:** Las cartas definen Reglas (Rules), Condiciones (ej: plan==FLEE o enemy\_tag(CAVALRY)) y Efectos con hooks (HOOK\_EXEC\_ROLL, HOOK\_MAP\_VISION).

* **Slots Limitados:** El General puede poseer 3 cartas benditas y 4 maldiciones activas. Su recambio se realiza dinámicamente, y las maldiciones/bendiciones tienen contadores de Duration (por Días o por Combates).

### 3.5. Sistema de Combate Táctico

Una máquina de estados de Múltiples Rondas que simula el choque asimétrico de fuerzas.

* **Fase 1 - Evaluación (Exploración):** El jugador tira 2d6 + Evaluación. Un éxito alto determina qué tanto del enemigo puede destapar (Tags y Estadísticas Ocultas).

* **Fase 2 - Planificación:** El jugador tira 2d6 para elegir qué maniobra táctica aplicar (Asalto Frontal, Escaramuza, Huida). El nivel de éxito bloqueará o desbloqueará las opciones avanzadas.

* **Fase 3 - Ejecución (Ventaja Logarítmica):** El daño nunca se calcula linealmente, para evitar abusos matemáticos por diferencia de nivel. Se calcula la **Ventaja de Fuerza Logarítmica**: Round(Log2(FuerzaPropia / FuerzaEnemiga) \* 3). Esto permite que incluso grandes ejércitos puedan sufrir desgastes contra fuerzas menores atrincheradas si no eligen bien el plan de ataque o si el dado (2-12) les da la espalda.

* **Fase 4 - Resolución:** El desgaste se inflige usando % sobre el *Peso del Ejército*. Se eliminan primero los Iniciados, luego los Soldados, y finalmente los Sargentos de élite.

### 3.6. Sistema Narrativo Contextual (Event Engine V2)

Separa totalmente a los programadores de los guionistas, utilizando una arquitectura *Data-Driven*.

* **Archivos .evt (Eventos) y .res (Resultados):** Las historias y dilemas contienen condiciones de detonación (MinInfamy, MinGold, Time) y un Trigger (ROAD, MARKET, CITY, NIGHT). Los resultados no están atados a la opción: una opción "Atacar" posee un Result\_pool permitiendo victorias gloriosas o masacres según RNG.

* **Seguridad UX (Cooldown):** Al dispararse un evento, se bloquean los clics en la UI durante 1.0 segundo para evitar accidentes en dobles clics rápidos de movimiento.

***

## 4. Finalidad y Bucle Jugable (Core Loop)

El diseño busca un ciclo de recompensa basado en riesgo y supervivencia:

1. **Evaluar Terreno:** Tirar los dados para saber cuántos PA posees y trazar una ruta considerando el clima.
2. **Explorar y Descubrir:** Empujar el FoW, encontrarse con tropas enemigas patrullando y decidir si emboscar o huir.
3. **Drafting (Cartas/Slots):** Visitar monolitos/altares para adquirir Modificadores que rompan las reglas de juego.
4. **Combate Táctico:** Destruir batallones con el sistema de Evaluación, Planificación y Ejecución asimétrica.

***

### 3.7. Reloj, Supervivencia e Inanición (DayClock & Survival)
* **Reloj Canónico (`DayClock`):** Cuatro franjas horarias por jornada (Mañana, Mediodía, Tarde, Noche). Controla unívocamente los eventos de cambio de fase y cierre del día.
* **Escalera de Inanición y Deshidratación:** Si al cerrar el día la comida o el agua están en 0, se incrementa el contador de días sin suministro. Al día 1 hay gracia; al día 2 cae la moral; al día 3 y posteriores hay pérdida severa de salud y bajas/deserciones escalonadas por rango (Iniciados primero).
* **Pronóstico de Raciones Unificado (`SupplyForecast`):** El HUD consulta directamente a `Army_ForecastSupply` una proyección franja a franja con el peor bioma del entorno. Muestra el rango $min - max$ días, derivando el estado (OK $\ge 3.0$, Escasa $1.5 - 3.0$, Crítica $< 1.5$) estrictamente de los días mínimos.
* **Forrajeo Calibrado:** Forrajear avanza el reloj hasta la tarde y rinde neto positivo ($\ge +0.25$ días de raciones) en biomas fértiles (Llanura, Bosque), con hover explicativo de rendimientos y coste en el HUD.
* **Sacrificio de Ganado y Pactos Divinos:** Opciones de emergencia para obtener raciones o agua a cambio de ganado o contrayendo maldiciones con deidades.

### 3.8. Campamento y Ciclo Nocturno
* **Elección al Atardecer:** Al finalizar la tarde es obligatorio decidir entre acampar o realizar marcha nocturna (con acampar como opción por defecto confirmable con ENTER).
* **Acampar:** Detiene el avance, recupera moral y salud (salvo inanición severa) y realiza tirada de emboscada ($2d6 + \text{Evaluación} + \text{Cobertura Terrestre}$). Al amanecer (`TIME_MORNING`) el estado de campamento se resetea limpiamente permitiendo tirar PA y moverse con normalidad.
* **Marcha Nocturna:** Permite seguir avanzando con coste de movimiento multiplicado fijo ($\times 1.5$) y penalidades acumulativas de salud y moral por noches consecutivas sin descanso.

### 3.9. IA Enemiga y Zona de Control (EnemySquad AI)
* **Máquina de Estados de Comportamiento:** Patrulla, Guardia, Cazador, Emboscador y Bestia. Estados de alerta, persecución mediante pathfinding hexagonal y retorno a base.
* **Zona de Control (ZOC):** Salir de un hexágono adyacente a un enemigo cuesta $+1$ PA adicional.
* **Ventaja de Huida (`skipNextPhase`):** Tras huir o ser derrotado, el enemigo entra en persecución pero salta su siguiente turno, evitando bloqueos injustos.
* **Cazador Periódico:** Un escuadrón rastreador aparece cada $N$ días en la retaguardia para presionar al ejército si demora demasiado.

### 3.10. Asentamientos, Mercado y Servicios
* **Interacción No Modal:** Al pasar cerca de un asentamiento se muestra un aviso interactivo (`[E]`), sin interrumpir la marcha salvo que el jugador lo elija.
* **Mercado Real:** Tienda con stock finito por visita generado determinísticamente. Precios de venta estrictamente menores que los de compra ($40\%$).
* **Armería y Ascensos:** Promoción de Iniciados a Soldados y Soldados a Sargentos con estricta conservación de tropas y ratio máximo ($\text{Sargentos} \le \text{Soldados}/15$).
* **Servicios Civiles:** Hospital para curar salud persistente por oro, Templo para purificar maldiciones (costo progresivo escalado según maldiciones purificadas) y Posada para restaurar moral consumiendo el día.

### 3.11. Sistema de Facciones y Asedio
* **Tres Facciones por Nivel (Asignación Espacial):** 3 semillas espaciales distribuidas en el mapa definen las áreas de influencia (diagrama de Voronoi) para cada asentamiento urbano, con temperamentos $(-1, 0, +1)$ que reaccionan a la Infamia del jugador.
* **Niveles de Opinión:** Hostil ($\le -50$, puertas cerradas), Desconfiado ($-49..-20$, precios $\times 1.5$), Neutral, Amistoso ($+21..+50$, precios $\times 0.85$) y Aliado ($> +50$, precios $\times 0.70$ y posada gratis).
* **Asedio a Guarniciones Fortificadas:** Opción en Castillos y Centros Urbanos. Dispara alerta de guardia, combate especial contra guarnición fortificada (`TAG_FORTIFIED`, 150 Salud, 6 rondas) y otorga botín triplicado ($\times 3.0$ mediante `CombatContext.lootMultiplier`), infamia ($+15$) y caída de reputación ($-40$).

***

## 4. Finalidad y Bucle Jugable (Core Loop)

El diseño busca un ciclo de recompensa basado en riesgo y supervivencia:

1. **Evaluar Terreno y Recursos:** Tirar los dados para saber cuántos PA posees, vigilar las raciones de comida y agua para no caer en la escalera de inanición.
2. **Explorar y Descubrir:** Empujar el FoW, calcular rutas sorteando o emboscando a patrullas enemigas con IA activa.
3. **Decisión al Atardecer:** Elegir entre la seguridad de acampar o la urgencia de marchar de noche.
4. **Comercio y Logística:** Visitar asentamientos para comerciar, promover soldados, curar el ejército o asediar plazas hostiles.
5. **Combate Táctico:** Destruir batallones con el sistema de Evaluación, Planificación y Ejecución asimétrica.

***

## 5. Estado de Implementación (Roadmap)

* [x] **Drafting UI (Fase E):** Sistema interactivo visual para que el jugador elija Cartas Divinas y Maldiciones en Altares.
* [x] **Supervivencia y Reloj (Fase 1):** `DayClock`, contadores de inanición/sed, `DayReport`, forrajeo y sacrificio de ganado.
* [x] **Campamento y Marcha Nocturna (Fase 2):** Decisión obligatoria vespertina, eventos nocturnos y combate con emboscada.
* [x] **IA Enemiga y Movimiento (Fase 3):** Estados de IA, persecución, ZOC (+1 PA), salto de fase post-huida y Cazador de retaguardia.
* [x] **Ciudades y Mercados (Fase 4):** Menú no modal, tienda con stock determinista, armería con ascensos $1:15$, hospital, templo y posada.
* [x] **Facciones y Asedio (Fase 5):** 3 facciones, modificadores de precios, niveles de alerta y combate de asedio fortificado con botín $\times 3$.
* [x] **Bloque A - Correcciones Playtest:**
  * A1: Desbloqueo de movimiento matutino tras acampar y unificación del reloj.
  * A2: Calibración de rendimientos de forrajeo neto positivo en Llanura/Bosque con tooltip HUD.
  * A3: Fuente única de pronóstico de suministros (`Army_ForecastSupply`) y umbrales unificados.
  * A4: Limpieza de combate (desfatigado ronda 1, centrado de botones, UI_Shutdown/UI_ButtonClicked).
  * A5: Asignación espacial de facciones Voronoi, botín de asedio integrado en `CombatContext`, escalado de purificación en templos por `cursesRemovedCount`.
* [x] **Bloque B - Legibilidad de la Interfaz:**
  * B1: Glosario unificado (`assets/glossary.cfg`, `glossary.h`, `glossary.c`) con 33 términos validados al inicio y tooltips con retardo de 0.35s.
  * B2: Tarjetas interactivas de plan en combate (`CombatPlanInfo`, `Combat_GetPlanInfo`) con desglose de coste, chips de efecto, flecha de ventaja vs intención y estado de fatiga en cuadrícula 2x2.
  * B3: Chips de glosario y pistas de primer uso descartables en combate, campamento y ciudad sin superposiciones a 1280x720 y 1600x900.
* [x] **Bloque C - Balance y Simulador de Cruce:**
  * C1: Principio de balance expresado en días de suministro.
  * C2 & C3: Simulador de cruce de nivel con piloto automático en C (`tests/sim_crossing.c`) sobre 4.000 mapas procedurales ($N=1.000$ por configuración de 3, 4, 5 y 6 días). Validación de metas C2 con 5.0 días: victoria 100%, salud 95.3/100, reservas finales 31.9% (meta: 15% a 35%). Botín de asedio garantizado (5 días de raciones y 200 de oro).
  * C4: Retuneo nocturno y simulador ampliado (`tests/sim_night_policy.c`): marcha sin cartas $\Delta = -11.8$ utilidad y salud $60.6 \ge 60$; marcha con 1 carta $\Delta = +9.3$; política mixta superior (337.4 pts); tabla de sensibilidad $\pm 50\%$ validada.
* [ ] **Bloque D - Descubrir Asentamientos:** Carteles / indicaciones en mapa, espaciado garantizado.
* [ ] **Bloque E - Oficiales y Motín:** Visibilidad y utilidad de tenientes, mecánica de motín recuperable.
* [ ] **Bloque F - Sensación de la Exploración.**
