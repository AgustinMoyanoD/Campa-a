[MODIFIER]
Id=curse_famine
Name=Maldicion de Hambruna
Description=Una plaga de langostas devora los sacos de grano tras la batalla. Se pierden 20 raciones de comida al concluir el combate por 3 batallas.
Kind=CURSE
Source=TARTARO
Rarity=RARE
Target=ARMY
Tags=MALDICION, HAMBRUNA
Duration=BATTLES:3

[RULE]
Hook=COMBAT_END
Effect=food:-20
