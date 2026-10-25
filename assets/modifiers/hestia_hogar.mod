[MODIFIER]
Id=hestia_hogar
Name=Llama de Hestia
Description=El calor del fuego sagrado restaura el espiritu y cura a los heridos graves. Otorga +30 de Moral tras victoria y reduce 30% las muertes definitivas.
Kind=CARD
Source=HESTIA
Rarity=RARE
Tags=MORAL, HOGAR
Duration=PERMANENTE

[RULE]
Hook=COMBAT_END
Effect=moral:+30

[RULE]
Hook=RECOUNT_DEATH
Effect=casualties:-30%
