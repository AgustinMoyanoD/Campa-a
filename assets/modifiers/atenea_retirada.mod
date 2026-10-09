[MODIFIER]
Id=atenea_retirada
Name=Retirada Estudiada
Kind=CARD
Source=ATENEA
Rarity=RARE
Tags=FLEE, PLANNING

[RULE]
Hook=PLAN_OPTIONS
Effect=unlock_plan:ORDERLY_RETREAT

[RULE]
Hook=CASUALTIES
Condition=plan==FLEE
Effect=casualties:-30%
