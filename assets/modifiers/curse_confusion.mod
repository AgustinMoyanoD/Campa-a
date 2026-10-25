[MODIFIER]
Id=curse_confusion
Name=Maldicion de Confusion
Description=Susurros de locura aturden a los oficiales impidiendo coordinar ordenes. Reduce en 4 la tirada de Planificacion tactica durante 3 batallas.
Kind=CURSE
Source=TARTARO
Rarity=RARE
Target=COMMANDER
Tags=MALDICION, TACTICA
Duration=BATTLES:3

[RULE]
Hook=PLAN_ROLL
Effect=planning:-4
