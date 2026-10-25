[MODIFIER]
Id=curse_cyclops
Name=Maldicion del Ciclope
Description=Un ojo vaciado ciega parte de la vision de mapa (-2), pero una colera ciega incrementa +2 la tirada de choque en combate por 5 batallas.
Kind=CURSE
Source=TARTARO
Rarity=RARE
Target=COMMANDER
Tags=MALDICION, VISION
Duration=BATTLES:5

[RULE]
Hook=MAP_VISION
Effect=vision:-2

[RULE]
Hook=EXEC_ROLL
Effect=execution:+2
