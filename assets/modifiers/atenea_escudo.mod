[MODIFIER]
Id=atenea_escudo
Name=Egida de Atenea
Description=El escudo impenetrable de la diosa de la estrategia resguarda las falanges. Reduce 40% el dano recibido y concede +3 a la Planificacion.
Kind=CARD
Source=ATENEA
Rarity=EPIC
Tags=DEFENSA, ESTRATEGIA
Duration=PERMANENTE
Rival=ARES
Invocation_Name=Muro de Falange
Invocation_Desc=Defensa inquebrantable de baja varianza: Mitiga 60% del dano recibido (+4 tirada). La carta se agota.
Invocation_Roll=4
Invocation_Clash=0.4
Invocation_Variance=LOW
Invocation_Counter=CHARGE

[RULE]
Hook=HEALTH_DAMAGE_RECEIVED
Effect=casualties:-40%

[RULE]
Hook=PLAN_ROLL
Effect=planning:+3
