[MODIFIER]
Id=ares_furia
Name=Furia de Ares
Description=La sed de sangre del dios de la guerra enciende el coraje de los tuyos. Otorga +5 a la tirada de Ejecucion en choque y +30% de dano.
Kind=CARD
Source=ARES
Rarity=RARE
Tags=COMBATE, AGRESIVA
Duration=PERMANENTE
Rival=ATENEA
Invocation_Name=Carga Sangrienta
Invocation_Desc=Embestida colosal de alta varianza: +8 a la tirada, x1.8 de choque. La carta se agota tras usarse.
Invocation_Roll=8
Invocation_Clash=1.8
Invocation_Variance=HIGH
Invocation_Counter=HOLD

[RULE]
Hook=EXEC_ROLL
Effect=execution:+5

[RULE]
Hook=EXEC_DAMAGE
Effect=execution:+30%
