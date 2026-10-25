[MODIFIER]
Id=atenea_retirada
Name=Retirada de Atenea
Description=La sabiduria tactica evita derrotas costosas. Desbloquea la Retirada Ordenada, suprime penalizaciones por huida y suma +4 a la Planificacion.
Kind=CARD
Source=ATENEA
Rarity=RARE
Tags=PLANIFICACION, HUIDA
Duration=PERMANENTE

[RULE]
Hook=PLAN_OPTIONS
Effect=unlock_plan:ORDERLY_RETREAT

[RULE]
Hook=FLEE_COST
Effect=flee_cost:-100%

[RULE]
Hook=PLAN_ROLL
Effect=planning:+4
