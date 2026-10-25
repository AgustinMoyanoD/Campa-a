[MODIFIER]
Id=apolo_sol
Name=Luz Radiante de Apolo
Description=La mirada profetica y clarividente del dios de la luz. Concede +5 a la tirada de Evaluacion de tropas enemigas y expande +2 el radio de vision.
Kind=CARD
Source=APOLO
Rarity=RARE
Tags=VISION, PROFECIA
Duration=PERMANENTE
Rival=DIONISIO
Invocation_Name=Centella Sagrada
Invocation_Desc=Ciega a las vanguardias rivales: +5 a la tirada y mitiga 40% del choque enemigo. La carta se agota.
Invocation_Roll=5
Invocation_Clash=0.6
Invocation_Variance=LOW
Invocation_Counter=CHARGE

[RULE]
Hook=EVAL_ROLL
Effect=evaluation:+5

[RULE]
Hook=MAP_VISION
Effect=vision:+2
