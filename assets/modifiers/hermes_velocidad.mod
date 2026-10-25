[MODIFIER]
Id=hermes_velocidad
Name=Marcha Ligera de Hermes
Description=Las tropas marchan incansables como el viento de Tracia. Otorga +1 Punto de Accion (PA) diario y anula por completo el coste de huida.
Kind=CARD
Source=HERMES
Rarity=RARE
Tags=MOVILIDAD, HUIDA
Duration=PERMANENTE

[RULE]
Hook=MAP_BASE_PA
Effect=pa:+1

[RULE]
Hook=FLEE_COST
Effect=flee_cost:-100%
