[MODIFIER]
Id=curse_styx_thirst
Name=Sed de la Estigia
Description=El agua negra del rio Estigia debilita los musculos y el pulso militar. Resta 3 puntos a la tirada de Ejecucion durante 5 dias.
Kind=CURSE
Source=TARTARO
Rarity=RARE
Target=ARMY
Tags=MALDICION, DEBILIDAD
Duration=DAYS:5

[RULE]
Hook=EXEC_ROLL
Effect=execution:-3
