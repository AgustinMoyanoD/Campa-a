[MODIFIER]
Id=ares_marea_roja
Name=Marea Roja de Ares
Description=Un torrente de violencia imparable aplasta al enemigo mientras el manto de la guerra mitiga las bajas definitivas de tu ejercito.
Kind=CARD
Source=ARES
Rarity=LEGENDARY
Tags=COMBATE, ANIQUILACION
Duration=PERMANENTE

[RULE]
Hook=EXEC_DAMAGE
Effect=execution:+60%

[RULE]
Hook=RECOUNT_DEATH
Effect=casualties:-35%
