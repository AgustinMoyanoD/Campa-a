# Manifiesto de Archivos de Audio

Este documento detalla todos los recursos de audio (música y efectos) esperados por el juego de campaña táctica.
El motor soporta formatos `.mp3`, `.ogg` y `.wav` (probados en ese orden de prioridad).
Los nombres deben ser en **minúsculas con guiones bajos**, sin espacios ni caracteres especiales.
Cualquier archivo faltante se mantendrá en silencio seguro sin romper la ejecución.

---

## 1. Música (`assets/music/`)

Todas las pistas se transmiten en streaming y se reproducen de forma completa en bucle continuo mientras no cambie el contexto. Al cambiar de contexto se aplica un fundido progresivo configurado en `assets/audio.cfg` (`FadeSeconds`).

| Archivo base (sin ext) | Carpeta | Formatos válidos | Dónde se usa en el juego | Imprescindible | Notas de comportamiento |
| :--- | :--- | :--- | :--- | :---: | :--- |
| `menu_theme` | `assets/music/` | `.mp3`, `.ogg`, `.wav` | Menú principal de inicio del juego (`PHASE_MENU`). | **Sí** | Suena en bucle al iniciar y al volver al menú. |
| `main_theme` | `assets/music/` | `.mp3`, `.ogg`, `.wav` | Exploración del mapa táctico (`PHASE_PLAYER`, `PHASE_ENEMY`). | **Sí** | **Se pausa** (no se reinicia) al entrar a combate o ciudad y **retoma donde quedó** al salir. |
| `combat_theme` | `assets/music/` | `.mp3`, `.ogg`, `.wav` | Encuentros de combate contra escuadras y asedios. | **Sí** | Suena con fundido de entrada al abrir la pantalla de combate. |
| `city_theme` | `assets/music/` | `.mp3`, `.ogg`, `.wav` | Asentamientos, pueblos y ciudades (`PHASE_CITY`). | **Sí** | Suena con fundido al ingresar a la vista de ciudad o campamento urbano. |

---

## 2. Efectos de Sonido (`assets/sfx/`)

### 2.1. Efectos Imprescindibles

| Archivo base (sin ext) | Carpeta | Formatos válidos | Dónde se usa en el juego | Imprescindible | Variantes / Fallback |
| :--- | :--- | :--- | :--- | :---: | :--- |
| `combat_start` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Al entrar en combate táctico desde el mapa. | **Sí** | Disparado al iniciar `CombatView`. |
| `combat_end` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Fin de combate en el paso `STEP_COMBAT_END` de la secuencia animada. | **Sí** | Sirve de fallback si no existen las variantes específicas abajo. |
| `combat_end_victory` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Fin de combate con victoria del jugador. | Opcional | Si no existe, reproduce `combat_end`. |
| `combat_end_defeat` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Fin de combate con aniquilación o derrota del jugador. | Opcional | Si no existe, reproduce `combat_end`. |
| `combat_end_flee` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Fin de combate tras una retirada/huida exitosa. | Opcional | Si no existe, reproduce `combat_end`. |
| `damage_dealt` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Al asestar daño al enemigo durante el paso `STEP_DAMAGE` del log. | **Sí** | Sincronizado exactamente con la animación de impacto y número flotante. |
| `market_buy` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Compra exitosa en el mercado (comida, agua, ganado, reclutas). | **Sí** | Se reproduce **únicamente** si la transacción se concreta con éxito. |
| `divine_card_acquired`| `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | En el altar divino, cuando la carta queda efectivamente equipada. | **Sí** | **No** suena al abrir el altar ni al ofrendar, solo al consolidar la carta. |

---

### 2.2. Efectos Frecuentes y Momentos Clave Propuestos

| Archivo base (sin ext) | Carpeta | Formatos válidos | Dónde se usa en el juego | Imprescindible | Notas de diseño |
| :--- | :--- | :--- | :--- | :---: | :--- |
| `damage_taken` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Al recibir daño las tropas del jugador en el choque táctico. | No | Sincronizado en la secuencia de daño del log. |
| `critical_hit` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Al obtener impacto crítico táctico (diferencial alto en dados). | No | Acompaña flash visual y sacudida de pantalla. |
| `dice_roll` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Lanzamiento de dados en el HUD y en las fases de combate. | No | Tono natural (sin variación de pitch). |
| `dice_land` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Detención y resultado final de los dados. | No | Tono natural (sin variación de pitch). |
| `ui_click` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Navegación genérica de interfaz (pestañas de ciudad, entrar/salir de distrito, abrir/cerrar paneles de oficiales o stats). | No | Ya no pisa los botones de compra, altar, combate ni menú principal. |
| `menu_click` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Botones del menú principal (`Nueva Campaña`, `Opciones`, `Salir`, `Volver`). | No | Separado de `ui_click` para darle identidad propia al menú de inicio. |
| `ui_confirm` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Botones de confirmación y avance de fase (`Ejecutar Choque`, `Pasar a Fase 2`, `Abandonar Ciudad`, `Continuar` en reportes/eventos). | No | Separado de `ui_click` para confirmar pasos tácticos y modales. |
| `ui_hover` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Al pasar el cursor sobre botones interactivos. | No | Volumen sutil (0.4) y limitador rápido. |
| `step_move` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Avance del ejército por cada hexágono del mapa. | No | Tono natural (sin variación). Admite variantes `step_<bioma>` si existen. |
| `turn_end` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Al pulsar `Pasar Turno` o `Atardecer` en el HUD. | No | Feedback auditivo exclusivo de fin de turno. |
| `forage` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Al pulsar `Forrajear (F)` en el HUD durante el día. | No | Sonido dedicado a recolectar provisiones en el terreno. |
| `camp_rest` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Al elegir `Opción A: Acampar` en la decisión de la tarde. | No | Sonido de armar campamento / fogata de descanso. |
| `night_march` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Al elegir `Opción B: Marcha Nocturna` en la decisión de la tarde. | No | Sonido de marcha forzada en la oscuridad. |
| `night_fall` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Transición del reloj hacia la franja horaria nocturna. | No | Tono ambiental oscuro / llamada nocturna. |
| `event_open` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Despliegue de un evento narrativo en el mapa. | No | Apertura solemne de pergamino/decisión. |
| `event_choice` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Al elegir una opción de decisión dentro de un evento narrativo. | No | Separado de `ui_click` para decisiones narrativas. |
| `market_sell` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Venta exitosa de provisiones o ganado en el mercado. | No | Sonido exclusivo de venta (sin `ui_click` encima). |
| `market_denied` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Intento de compra, venta o servicio denegado (sin oro o sin requisitos). | No | Sonido exclusivo de rechazo (sin `ui_click` encima). |
| `altar_offer` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Al pulsar `Ofrendar Oro` en el Altar Divino para revelar una bendición. | No | Separado de `ui_click` y de `divine_card_acquired`. |
| `curse_acquired` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Al pulsar `Aceptar Maldición` en un pacto o evento oscuro. | No | Clímax oscuro exclusivo (sin `ui_click` encima). |
| `card_sacrificed` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Al descartar/reemplazar una bendición previa en el altar. | No | Destrucción o despedida del favor divino. |
| `promote_troops` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Botones de ascenso en la armería (Iniciados a Soldados, Soldados a Sargentos). | No | Choque marcial exclusivo (sin `ui_click` encima). |
| `heal` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Botón de curación en el hospital militar o purificación en el templo. | No | Alivio / bendición curativa exclusiva (sin `ui_click` encima). |
| `inn_rest` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Botón de banquete y descanso en la Posada (`Subir Moral`). | No | Ambiente de taberna / brindis de la tropa (sin `ui_click` encima). |
| `siege_start` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Botón `Asediar y Saquear` para iniciar el asalto a una ciudad. | No | Cuerno de guerra / tambor de asalto (sin `ui_click` encima). |
| `siege_victory` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Conquista y saqueo victorioso de una ciudad asediada. | No | Fanfarria militar de conquista. |
| `enemy_alert` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Al ser interceptado por una patrulla enemiga o emboscada. | No | Señal de alerta / peligro inmediato. |
| `low_supply_warning` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Cuando los suministros de comida o agua caen a nivel crítico (< 2 días). | No | Sonido de advertencia de desabastecimiento. |
| `mutiny_warning` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Al detonar advertencias telegrafiadas de motín de la tropa. | No | Murmullo hostil / aviso de rebelión. |
| `level_complete` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Al alcanzar la casilla de salida y conquistar la provincia. | No | Fanfarria de victoria de nivel. |
| `game_over` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Al ser aniquilado el ejército del jugador. | No | Toque fúnebre / derrota definitiva. |
| `ambient_crow` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Graznido de cuervo aleatorio en el mapa (día/tarde/noche). | No | Sonido ambiental esporádico. Fallback: `crow`, `cuervo`. |
| `ambient_wind` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Ráfaga de viento ambiental en el mapa. | No | Sonido ambiental esporádico. Fallback: `wind`, `viento`. |
| `ambient_birds` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Trino de aves o pájaros diurnos (mañana/mediodía/tarde). | No | Sonido ambiental diurno. Fallback: `birds`, `pajaros`, `bird`. |
| `ambient_insects` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Grillos e insectos nocturnos (noche). | No | Sonido ambiental nocturno. Fallback: `insects`, `insectos`, `crickets`, `grillos`. |
| `ambient_animals` | `assets/sfx/` | `.mp3`, `.ogg`, `.wav` | Aullidos o fauna distante (día/noche). | No | Sonido ambiental salvaje. Fallback: `animals`, `animales`, `wolf`, `lobo`. |

---

## 3. Guía de Subida para Desarrolladores

1. Copiar los archivos en `assets/audio/music/` y `assets/audio/sfx/` (o `assets/music/` y `assets/sfx/`).
2. Comprobar que los nombres coincidan exactamente con la columna **Archivo base**.
3. Si querés que dos categorías compartan el mismo archivo sin duplicarlo en disco, podés cambiar `File=...` en `assets/audio.cfg` dentro de la sección `[SFX_...]` correspondiente.
4. Al iniciar el juego, revisar `game_log.txt` o la salida en consola: el sistema reportará una lista inicial de todos los archivos esperados que aún no se han subido.
