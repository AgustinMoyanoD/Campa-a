[MODIFIER]
Id=curse_panic
Name=Maldicion de Panico
Description=El terror ancestral de Fobos y Deimos sacude a la soldadesca. Multiplica el desorden y coste al huir (+30) durante 3 batallas.
Kind=CURSE
Source=TARTARO
Rarity=RARE
Target=ARMY
Tags=MALDICION, PANICO
Duration=BATTLES:3

[RULE]
Hook=FLEE_COST
Effect=flee_cost:+30
