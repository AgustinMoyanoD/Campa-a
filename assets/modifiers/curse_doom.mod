[MODIFIER]
Id=curse_doom
Name=Maldicion de Perdicion
Description=Las tres Parcas cortan implacablemente el hilo de la vida. Aumenta un 40% las muertes definitivas tras cada combate durante 3 batallas.
Kind=CURSE
Source=TARTARO
Rarity=EPIC
Target=ARMY
Tags=MALDICION, MUERTE
Duration=BATTLES:3

[RULE]
Hook=RECOUNT_DEATH
Effect=casualties:+40%
