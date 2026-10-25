[MODIFIER]
Id=curse_plague
Name=Maldicion de Peste
Description=Fiebres malignas y miasmas debilitan la constitucion de los hoplitas. Incrementa un 30% las bajas recibidas en combate por 3 batallas.
Kind=CURSE
Source=TARTARO
Rarity=RARE
Target=ARMY
Tags=MALDICION, PESTE
Duration=BATTLES:3

[RULE]
Hook=HEALTH_DAMAGE_RECEIVED
Effect=casualties:+30%
