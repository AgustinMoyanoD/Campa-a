[MODIFIER]
Id=hecate_sombra_guiadora
Name=Sombra Guiadora de Hecate
Kind=CARD
Source=HECATE
Rarity=EPIC
Tags=NIGHT, EVENT, INFAMY

[RULE]
Hook=AMBUSH_ROLL
Effect=evaluation:+2

[RULE]
Hook=EVENT_RESULT
Condition=Time==NIGHT
Effect=infamy:+2
