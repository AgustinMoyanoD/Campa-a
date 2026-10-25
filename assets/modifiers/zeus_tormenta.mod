[MODIFIER]
Id=zeus_tormenta
Name=Tormenta Olimpica
Description=Relampagos celestiales calcinan las formaciones enemigas. Incrementa un 50% el dano de choque infligido y suma +3 a las tiradas de Ejecucion.
Kind=CARD
Source=ZEUS
Rarity=EPIC
Tags=COMBATE, DANO
Duration=PERMANENTE

[RULE]
Hook=EXEC_DAMAGE
Effect=execution:+50%

[RULE]
Hook=EXEC_ROLL
Effect=execution:+3
