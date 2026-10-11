[MODIFIER]
Id=curse_tantalus_hunger
Name=Maldición de Tántalo
Description=Una bruma sepulcral reduce el alcance de visión en el mapa por 5 días.
Kind=CURSE
Source=HADES
Rarity=RARE
Target=ARMY
Duration=DAYS:5
Tags=VISION, EXPLORATION

[RULE]
Hook=MAP_VISION
Effect=vision:-2
