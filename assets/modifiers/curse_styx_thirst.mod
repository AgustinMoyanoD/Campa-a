[MODIFIER]
Id=curse_styx_thirst
Name=Maldición de la Estigia
Description=El agua negra debilita los músculos reduciendo el choque de combate por 5 días.
Kind=CURSE
Source=HADES
Rarity=RARE
Target=ARMY
Duration=DAYS:5
Tags=DAMAGE, COMBAT

[RULE]
Hook=EXEC_ROLL
Effect=execution:-2
