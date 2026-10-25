[MODIFIER]
Id=hermes_pie_alado
Name=Pie Alado de Hermes
Description=La velocidad divina del mensajero del Olimpo impregna las marchas. Otorga +2 Puntos de Accion (PA) diarios y reduce en 1 el coste de movimiento en mapa.
Kind=CARD
Source=HERMES
Rarity=LEGENDARY
Tags=MOVILIDAD, PA
Duration=PERMANENTE

[RULE]
Hook=MAP_BASE_PA
Effect=pa:+2

[RULE]
Hook=MAP_MOVE_COST
Effect=move_cost:-1
