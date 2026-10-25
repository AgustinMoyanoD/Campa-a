[MODIFIER]
Id=artemisa_caza
Name=Caceria de Artemisa
Description=El instinto depredador de la diosa virgen de los bosques. Otorga +4 para detectar y anular emboscadas en acampadas y +3 a la Planificacion tactica.
Kind=CARD
Source=ARTEMISA
Rarity=RARE
Tags=CAZA, EMBOSCADA
Duration=PERMANENTE

[RULE]
Hook=AMBUSH_ROLL
Effect=evaluation:+4

[RULE]
Hook=PLAN_ROLL
Effect=planning:+3
