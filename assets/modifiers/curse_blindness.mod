[MODIFIER]
Id=curse_blindness
Name=Maldicion de Ceguera
Description=Vapores sulfureos del inframundo nublan la vista del general. Reduce en 4 puntos la tirada de Evaluacion de tropas enemigas durante 3 batallas.
Kind=CURSE
Source=TARTARO
Rarity=RARE
Target=COMMANDER
Tags=MALDICION, VISION
Duration=BATTLES:3

[RULE]
Hook=EVAL_ROLL
Effect=evaluation:-4
