[MODIFIER]
Id=artemisa_ojo_nocturno
Name=Ojo Nocturno de Artemisa
Description=Vision penetrante que disipa la oscuridad nocturna. Incrementa +3 el radio de vision y otorga +3 a la Evaluacion de fuerzas enemigas.
Kind=CARD
Source=ARTEMISA
Rarity=EPIC
Tags=VISION, NOCHE
Duration=PERMANENTE

[RULE]
Hook=MAP_VISION
Effect=vision:+3

[RULE]
Hook=EVAL_ROLL
Effect=evaluation:+3
