[MODIFIER]
Id=selene_paso_plateado
Name=Paso Plateado de Selene
Description=La luna llena alumbra y allana senderos secretos. Reduce en 1 el coste de movimiento en mapa y expande +2 el radio de vision exploratoria.
Kind=CARD
Source=SELENE
Rarity=RARE
Tags=MOVILIDAD, NOCHE
Duration=PERMANENTE

[RULE]
Hook=MAP_MOVE_COST
Effect=move_cost:-1

[RULE]
Hook=MAP_VISION
Effect=vision:+2
