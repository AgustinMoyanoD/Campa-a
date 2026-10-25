[MODIFIER]
Id=poseidon_terremoto
Name=Terremoto de Poseidon
Description=La furia del Sacudidor de Tierras desquebraja las filas rivales. Anade +45% de dano de choque en combate y +3 a las tiradas de Ejecucion.
Kind=CARD
Source=POSEIDON
Rarity=EPIC
Tags=COMBATE, TERREMOTO
Duration=PERMANENTE
Rival=ZEUS
Invocation_Name=Maremoto Terrestre
Invocation_Desc=Sacudida devastadora: +7 a la tirada, x1.7 de choque contra maniobras enemigas. La carta se agota.
Invocation_Roll=7
Invocation_Clash=1.7
Invocation_Variance=HIGH
Invocation_Counter=MANEUVER

[RULE]
Hook=EXEC_DAMAGE
Effect=execution:+45%

[RULE]
Hook=EXEC_ROLL
Effect=execution:+3
