[MODIFIER]
Id=dionisio_vino
Name=Nectar de Dionisio
Description=La embriaguez sagrada disipa el cansancio y une a las filas. Anade +20 cantimploras de Vino de botin y +25 de Moral tras vencer en combate.
Kind=CARD
Source=DIONISIO
Rarity=RARE
Tags=VINO, MORAL
Duration=PERMANENTE

[RULE]
Hook=COMBAT_END
Effect=wine:+20

[RULE]
Hook=COMBAT_END
Effect=moral:+25
