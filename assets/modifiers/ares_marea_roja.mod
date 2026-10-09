[MODIFIER]
Id=ares_marea_roja
Name=Marea Roja de Ares
Kind=CARD
Source=ARES
Rarity=EPIC
Tags=DAMAGE, AGGRESSIVE

[RULE]
Hook=EXEC_DAMAGE
Effect=casualties:+50%

[RULE]
Hook=CASUALTIES
Effect=moral:-5
