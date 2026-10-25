[MODIFIER]
Id=hecate_sombra_guiadora
Name=Antorcha de Hecate
Description=La guardiana de las encrucijadas advierte toda asechanza nocturna. Concede +5 a la deteccion de emboscadas y otorga +1 Punto de Accion (PA) diario.
Kind=CARD
Source=HECATE
Rarity=EPIC
Tags=MAGIA, EMBOSCADA
Duration=PERMANENTE

[RULE]
Hook=AMBUSH_ROLL
Effect=evaluation:+5

[RULE]
Hook=MAP_BASE_PA
Effect=pa:+1
