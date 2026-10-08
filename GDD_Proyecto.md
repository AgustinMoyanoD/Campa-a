# Documento de Diseño de Proyecto (GDD): Campaña Estratégica Táctica

## 1. Visión General del Proyecto
**Título del Proyecto:** [Por definir] - Motor de Campaña Táctica
**Motor/Tecnología:** Lenguaje C puro + Raylib (Renderizado Gráfico)
**Género:** Estrategia por turnos, Gestión de Recursos, Rol (RPG) y Narrativa Emergente.

### Resumen
El proyecto es un simulador de **Campaña Militar Táctica** ambientado en un mundo de hexágonos isométricos generados proceduralmente. El jugador encarna al comandante de un ejército, navegando por terrenos desconocidos, gestionando tropas diferenciadas por rangos, midiendo provisiones, y tomando decisiones que alteran su **Infamia**. La finalidad es construir un motor altamente modular y escalable, donde la narrativa sea dictada por datos externos (Data-Driven) y las matemáticas tácticas dependan de los rangos de los soldados y las virtudes de sus comandantes.

---

## 2. Arquitectura de Software (Patrón MVC)
El código base está estrictamente organizado siguiendo un patrón **MVC (Modelo-Vista-Controlador)**. Esto evita el "código espagueti" en el bucle principal (main.c) y permite que el proyecto crezca sin romperse.

* **src/models/**: La lógica pura, matemática y reglas de negocio. No saben nada sobre cómo se dibujan las cosas en pantalla. (Ej: rmy.c, hex_tile.c, event.c).
* **src/views/**: Encargada estrictamente de dibujar en pantalla usando Raylib y escuchar el mouse/teclado. (Ej: map_view.c, event_view.c, hud.c).
* **src/controllers/**: El "puente" que conecta modelos y vistas. Toman decisiones basadas en las reglas. (Ej: game_master.c, camera_controller.c).

---

## 3. Análisis Profundo de los Sistemas Principales

### 3.1. Generación de Mundo y Terreno (MapGenerator)
El mapa no es estático; se genera mediante algoritmos procedurales asegurando que cada campaña sea única.
* **Geometría de Coordenadas Axiales:** Se usan ejes (Q, R) en lugar del tradicional (X, Y). Esto permite calcular distancias reales en forma de panal de abejas, determinando qué hexágonos son adyacentes de manera matemáticamente perfecta.
* **Distribución de Biomas:** El motor inyecta terrenos (Llanuras, Bosques, Montañas, Agua). Cada bioma expone un bloque de propiedades (BiomeProperties):
  * *Costo de PA:* Atravesar montañas cuesta más puntos de acción que un sendero.
  * *Consumo Climático:* Zonas desérticas o montañosas queman tus reservas de agua y comida el doble de rápido.

### 3.2. Renderizado Isométrico y Visión (MapView & FoW)
* **Perspectiva Isométrica Real:** Todo el mundo es renderizado usando una escala de profundidad Y * 0.5, logrando que la cuadrícula se acueste en un pseudo-3D. Los brillos de interacción se calculan con esta misma trigonometría.
* **Niebla de Guerra de 3 Capas (FoW):**
  1. *Desconocido:* Fuera del rango, no se dibuja (Ahorro crítico de memoria mediante *Dynamic Culling*).
  2. *Visitado (Memoria):* El terreno queda en un registro persistente ampliado (Visión + 2 hexágonos de buffer). Se dibuja oscurecido; sabes que hay una montaña, pero no si hay un enemigo en ella.
  3. *Visible (Línea de Visión):* Hexágonos iluminados a todo color alrededor del jugador.

### 3.3. El Orquestador del Juego (Game Master)
Es la "mesa de rol" virtual. Administra los límites del jugador.
* **Flujo de Puntos de Acción (PA):** Inicias en 0. Para poder moverte, debes tirar físicamente los dados (consumiendo la interacción del turno). El total de tus PA será: Base (ej: 3) + Cara del Dado 1 + Cara del Dado 2.
* **Ciclo de Tiempo:** Moverse o interactuar desgasta el día (Mañana -> Mediodía -> Tarde -> Noche). 
* **Penalidades Nocturnas:** Al llegar la Noche, la visibilidad de tu ejército se recorta (FoW dinámico), provocando que la oscuridad te encierre, aumentando el peligro de ser emboscado si decides seguir marchando sin descansar.
* **Sistema de Infamia:** Escala de 0 a 100%. Representa el odio o el miedo que el mundo te tiene. Se altera tomando decisiones amorales en los eventos, afectando la probabilidad futura de hostilidad de ciertas facciones.

### 3.4. Gestión del Ejército y Comandantes (Army System)
El ejército no es un solo "jugador" genérico, sino un cuerpo táctico con peso real y logística.
* **Ponderación de Tropas (Rangos):**
  * *Iniciados:* Carne de cañón, baratos de mantener pero débiles en impacto táctico.
  * *Soldados:* El grueso estándar militar.
  * *Sargentos:* Oficiales curtidos de alto valor.
  * La suma ponderada de estas unidades genera la Fuerza Total, que se usa como base para la fase de daño en los combates.
* **Logística (Agua, Comida y Ganado):** El consumo de raciones por turno no es arbitrario; es una fórmula proporcional al número total de soldados multiplicada por la dureza del bioma. El ejército puede arrear *Ganado*, el cual funciona como un "amortiguador" que reduce el costo neto de alimento, pero podría tener penalidades de movilidad.
* **El Estado Mayor (Stats de Oficiales):** El General y sus Tenientes proporcionan la columna vertebral técnica del ejército a través de 5 pilares:
  * *Visión:* Amplía el radio de hexágonos iluminados en el FoW.
  * *Movilidad:* Define los Puntos Base de PA que se sumarán a los dados.
  * *Evaluación (Fase 1 del Combate):* Capacidad de reconocimiento y de encontrar flancos débiles en las fuerzas enemigas al chocar.
  * *Planificación (Fase 2 del Combate):* Estrategia de terreno. Provee bonificaciones tácticas o escudos de vanguardia.
  * *Ejecución (Fase 3 del Combate):* El daño duro. Potencia multiplicadora para los tiros de dados de ataque (Fuerza = Ejecución + D12).

### 3.5. Sistema Narrativo Contextual (Event Engine V2)
Es el sistema más avanzado y flexible del proyecto. Separa totalmente a los programadores de los guionistas, utilizando una arquitectura *Data-Driven* guiada por Contextos y Pools de Resultados.
* **Archivos .evt (Eventos):** Las historias y dilemas se almacenan en texto estructurado. Contienen condiciones de detonación (MinInfamy, MinGold, Time, MinArmySize), un Trigger (ROAD, MARKET, CITY, NIGHT, etc.) y opciones para el jugador.
* **Archivos .res (Resultados):** A diferencia de un sistema rígido, las consecuencias no están atadas a la opción. Una opción posee un Result_pool (ej: es_vict, res_derr), permitiendo que elegir "Atacar" resulte en una victoria gloriosa o en una masacre según el RNG (y a futuro, según las stats del General). Los resultados pueden alterar Oro, Infamia, Moral, Comida y causar pérdidas específicas de tropas (Loss_Initiates, Loss_Soldiers).
* **Mecánica de Detonación y Filtrado:** Cuando el jugador avanza a una casilla, el *Game Master* construye un EventContext con la foto exacta de la situación (hora del día, infamia actual, oro, tipo de estructura pisada). El motor filtra de la memoria solo los eventos que coincidan con este contexto exacto, elige uno al azar entre los válidos, y finalmente lanza el dado de Probability (0-100%). Esto evita el "sesgo de cantidad" (donde tener 100 eventos de bosque ahogaría la probabilidad de que salga 1 de granja).
* **Seguridad UX (Cooldown):** Al dispararse un evento, se invoca a event_view.c. Este dibuja el pergamino pero bloquea los clics durante 1.0 segundo. Esto evita que el jugador seleccione una opción letal sin querer si venía haciendo doble-clics rápidos en el mapa.
* **Editor Web Local:** El Game Master incluye su propia herramienta gráfica (herramientas/editor_eventos.html) que permite a los diseñadores crear y vincular archivos .evt y .res sin tocar código, validando referencias y exportando/guardando los archivos formateados automáticamente.

---

## 4. Finalidad y Bucle Jugable (Core Loop)
El diseño busca un ciclo de recompensa basado en riesgo y supervivencia:
1. **Evaluar Terreno:** El jugador debe usar los dados para saber cuántos PA posee, y trazar una ruta considerando el clima (bosque vs llanura) para no deshidratar a su tropa.
2. **Explorar:** Empujar el FoW para hallar pueblos (donde gastar oro y reclutar Iniciados/Sargentos) u objetivos clave.
3. **Rol e Imprevistos:** Lidiar con el *Event Engine*. Elegir si saquear una caravana (ganando oro pero subiendo Infamia) o escoltarla (ganando moral).
4. **Combate Táctico (Fase Futura):** Emplear los stats de Evaluación y Ejecución del General para machacar a la escuadra enemiga en una pantalla de duelo de números y dados.

---

## 5. Próximos Pasos (Roadmap del Alpha)
- [ ] **Renderizado Enemigo:** Inyectar el EnemySquad modelado sobre el mapa para que patrullen caminos.
- [ ] **Resolución de Combate:** Crear la pantalla que choque las estadísticas de tu ejército contra la de los rivales al intentar pisar su hexágono.
- [ ] **Estructuras Interactivas y Reclutamiento:** Que hacer clic sobre una ciudad abra un "Mercado" guiado por el oro y las tropas.

