[MODIFIER]
Id=curse_cyclops
Name=Maldicion del Ciclope
Kind=CURSE
Target=COMMANDER
Tags=CURSE, VISION
Duration=BATTLES:5

[RULE]
Hook=MAP_VISION
Effect=vision:-2

[RULE]
Hook=EXEC_ROLL
Effect=execution:+2
