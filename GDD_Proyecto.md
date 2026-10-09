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

## 5. Próximos Pasos (Roadmap del Alpha)

* [ ] **Drafting UI (Fase E):** Sistema interactivo Visual para que el jugador elija Cartas Divinas y Maldiciones en Altares del mapa (Draft de 3 opciones y reemplazo de slots).

* [ ] **Estructuras Interactivas y Reclutamiento:** Que hacer clic sobre una ciudad abra un "Mercado" guiado por el oro y las tropas.

* [ ] **Patrullaje Automático de Enemigos:** Sistema de IA que permita a las escuadras enemigas gastar PA y moverse por los caminos asediando aldeas.
