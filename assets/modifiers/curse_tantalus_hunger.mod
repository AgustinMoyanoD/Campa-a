[MODIFIER]
Id=curse_tantalus_hunger
Name=Bruma de Tantalo
Description=Una bruma sepulcral y un espejismo asfixiante ciegan la marcha. Reduce en 2 el alcance de vision en el mapa por 5 dias.
Kind=CURSE
Source=TARTARO
Rarity=RARE
Target=ARMY
Tags=MALDICION, VISION
Duration=DAYS:5

[RULE]
Hook=MAP_VISION
Effect=vision:-2
