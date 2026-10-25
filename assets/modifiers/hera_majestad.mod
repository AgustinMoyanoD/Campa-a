[MODIFIER]
Id=hera_majestad
Name=Majestad de Hera
Description=La soberana del Olimpo impone una disciplina ferrea e impecable. Otorga +4 a la Evaluacion de tropas rivales y +4 a la Planificacion tactica.
Kind=CARD
Source=HERA
Rarity=EPIC
Tags=LIDERAZGO, TACTICA
Duration=PERMANENTE

[RULE]
Hook=EVAL_ROLL
Effect=evaluation:+4

[RULE]
Hook=PLAN_ROLL
Effect=planning:+4
