[MODIFIER]
Id=demeter_cosecha
Name=Cosecha de Demeter
Description=La fertilidad de la tierra sacia el hambre y la sed de tus tropas. Anade +50 raciones de Comida y +30 odres de Agua al botin tras cada victoria.
Kind=CARD
Source=DEMETER
Rarity=RARE
Tags=SUMINISTROS, COSECHA
Duration=PERMANENTE

[RULE]
Hook=COMBAT_END
Effect=food:+50

[RULE]
Hook=COMBAT_END
Effect=water:+30
