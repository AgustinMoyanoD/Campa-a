# Referencia de la API (Arquitectura MVC)

Este documento sirve como un índice rápido de los archivos de cabecera (.h) del proyecto, explicando brevemente su propósito dentro de la arquitectura MVC. (Se omiten las librerías externas de Raylib).

---

## 📂 Controladores (include/controllers/)
Actúan como el puente de reglas entre la visualización en pantalla y el procesamiento lógico.

### Asset_manager.h
- **Propósito:** Gestión de memoria de texturas e imágenes.
- **Estructuras:** GameAssets (Contiene las texturas de estructuras, unidades y enemigos).
- **Funciones clave:** AssetManager_LoadAll, AssetManager_UnloadAll. Evita cargar la misma textura múltiples veces en VRAM.

### camera_controller.h
- **Propósito:** Controla la cámara 2D de Raylib para navegar el mapa.
- **Funciones clave:** CameraController_Update (Zoom y paneo con WASD/Mouse), CameraController_FocusOn (Centra la vista en un hexágono específico, útil post-combate o en el inicio).

### game_master.h
- **Propósito:** El orquestador principal del estado del juego (la "Mesa de Rol").
- **Estructuras:** GameMaster (Contiene los PA, hora del día, infamia, estado del turno, el registro de modificadores y la lista de enemigos activos).
- **Funciones clave:** GameMaster_AttemptMove (Valida si un movimiento es legal), GameMaster_ApplyDiceRoll, GameMaster_GetVisionModifier (Evalúa el entorno y los hooks para dar visión extra o penalizarla).

---

## 📂 Modelos (include/models/)
Contienen las matemáticas puras, estructuras de datos y lógica dura. No tienen dependencias visuales de Raylib.

### Army.h
- **Propósito:** Simulación logística y militar del ejército del jugador.
- **Estructuras:** Army (Recursos, posición, tropas), ArmyStats (Evaluación, Planificación, Ejecución).
- **Funciones clave:** Army_GetStats (Calcula el poder duro de choque basándose en los generales y tropas), Army_EquipModifier / Army_RemoveModifier (Agrega o elimina cartas al comandante en jefe).

### combat.h
- **Propósito:** La máquina de estados asimétrica de combate.
- **Estructuras:** CombatContext (Guarda los dados, ventajas matemáticas, daño y planes seleccionados de la batalla activa).
- **Funciones clave:** Combat_RollEvaluation, Combat_RollPlanning, Combat_RollExecution, Combat_ApplyResolution. Controlan matemáticamente el daño logarítmico.

### effect.h
- **Propósito:** Parseador universal de comandos de efectos.
- **Estructuras:** EffectDef (Modo Absoluto o Porcentaje, tipo de stat afectado).
- **Funciones clave:** Effect_ParseString (Traduce strings como enemy.casualties:-50% a una estructura usable).

### enemy.h
- **Propósito:** Representación y lógica de patrullas y bandidos.
- **Estructuras:** EnemySquad (Fuerza base, movilidad, etiquetas TAGS y modificadores activos).
- **Funciones clave:** Enemy_RevealTags, Enemy_IsTagKnown (Controlan la visibilidad de información asimétrica en combate).

### event.h
- **Propósito:** El motor narrativo *Data-Driven*.
- **Estructuras:** EventDef (Evento cargado), EventContext (El contexto situacional al pisar una casilla).
- **Funciones clave:** Event_LoadAll, Event_CheckConditions, Event_ApplyResult (Distribuye botín, daño narrativo y cambios de infamia al elegir opciones en los textos).

### general.h
- **Propósito:** Estructuras de oficiales y polimorfismo.
- **Estructuras:** General (El avatar principal con sus cartas divinas), Lieutenant (Oficiales con lealtad y riesgo de motín), CommanderRef (Referencia polimórfica).
- **Funciones clave:** Commander_GetActiveModifier (Obtiene dinámicamente los efectos activos de un oficial sin importar su rango).

### hex_tile.h
- **Propósito:** Geometría axial base.
- **Estructuras:** HexCoord (Ejes Q, R), HexTile (Un bloque de terreno con su bioma e info especial).
- **Funciones clave:** HexCoord_Distance, HexCoord_GetNeighbors.

### map_generator.h
- **Propósito:** Generación procedural de mundos.
- **Estructuras:** Map (Arreglo de celdas).
- **Funciones clave:** Map_Generate, Map_UpdateVision (Calcula dinámicamente el Fog of War explorado basándose en la visión del ejército).

### modifier.h
- **Propósito:** El corazón técnico del sistema de Cartas y Buffs.
- **Estructuras:** ModifierRegistry (Catálogo en RAM de los .mod), ModifierDef, ModBreakdown (Desglose de matemáticas).
- **Funciones clave:** Modifier_Query (Interroga todos los efectos aplicables sobre un Hook, ej: HOOK_MAP_VISION y devuelve el número acumulado total).

---

## 📂 Vistas (include/views/)
Módulos encargados puramente de la interfaz de usuario (UI), renderizado en pantalla e interacciones de teclado/mouse.

### combat_view.h
- **Propósito:** Renderiza las 4 fases de combate. Interfaz gráfica de selección de planes, muestra de dados base y el renderizado complejo de los Tooltips de Desglose Matemático (ModBreakdown).

### enemy_view.h
- **Propósito:** Inyecta en el mapa isométrico 3D los avatares (tokens) de las patrullas enemigas ocultándolos inteligentemente tras el Fog of War.

### event_view.h
- **Propósito:** Dibuja el gran pergamino de texto centrado cuando un evento narrativo se dispara. Calcula las áreas de colisión (clics) para las opciones del jugador y provee un pequeño tiempo de "Cooldown" para evitar clics accidentales.

### hud.h
- **Propósito:** Interfaz "siempre encima". Dibuja los recursos logísticos, el botón para tirar dados de PA, y las cartas divinas/maldiciones activas en la esquina derecha del General.

### map_view.h
- **Propósito:** Motor gráfico del tablero principal. Dibuja cada hexágono en perspectiva *Y * 0.5*, oscurece con shaders/colores el Fog of War inexplorado, coloca sprites decorativos (estructuras), y levanta un mini-mapa localizador.
