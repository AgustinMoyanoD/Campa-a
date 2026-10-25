[MODIFIER]
Id=erida_discordia
Name=Discordia de Eris
Description=La manzana del caos desata el frenesi salvaje entre las lineas adversarias. Suma +4 a la tirada de Ejecucion y +35% de dano en choque victorioso.
Kind=CARD
Source=ERIS
Rarity=RARE
Tags=CAOS, COMBATE
Duration=PERMANENTE

[RULE]
Hook=EXEC_ROLL
Effect=execution:+4

[RULE]
Hook=EXEC_DAMAGE
Effect=execution:+35%
