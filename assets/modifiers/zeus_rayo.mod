[MODIFIER]
Id=zeus_rayo
Name=Rayo de Zeus
Description=El rey del Olimpo bendice a tus guerreros con el poder del trueno primordial. Otorga +4 a la tirada de Ejecucion y reduce 20% el dano recibido.
Kind=CARD
Source=ZEUS
Rarity=RARE
Tags=COMBATE, DIVINA
Duration=PERMANENTE
Rival=POSEIDON
Invocation_Name=Trueno Olimpico
Invocation_Desc=Impacto celestial: +6 a la tirada y choque x1.5. Rompe formaciones defensivas. La carta se agota.
Invocation_Roll=6
Invocation_Clash=1.5
Invocation_Variance=HIGH
Invocation_Counter=HOLD

[RULE]
Hook=EXEC_ROLL
Effect=execution:+4

[RULE]
Hook=HEALTH_DAMAGE_RECEIVED
Effect=casualties:-20%
