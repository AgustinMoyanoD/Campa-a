[MODIFIER]
Id=afrodita_encanto
Name=Encanto de Afrodita
Description=Una gracia irresistible desarma el odio de los contrincantes. Reduce 35% las bajas recibidas en combate y premia con +20 de Moral tras la victoria.
Kind=CARD
Source=AFRODITA
Rarity=EPIC
Tags=DEFENSA, MORAL
Duration=PERMANENTE

[RULE]
Hook=HEALTH_DAMAGE_RECEIVED
Effect=casualties:-35%

[RULE]
Hook=COMBAT_END
Effect=moral:+20
