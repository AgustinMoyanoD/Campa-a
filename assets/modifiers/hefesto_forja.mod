[MODIFIER]
Id=hefesto_forja
Name=Forja de Hefesto
Description=Espadas templadas en fuego divino y corazas reforzadas. Otorga +40% de dano de choque infligido y reduce 25% el dano sufrido en batalla.
Kind=CARD
Source=HEFESTO
Rarity=EPIC
Tags=ARMAS, FORJA
Duration=PERMANENTE

[RULE]
Hook=EXEC_DAMAGE
Effect=execution:+40%

[RULE]
Hook=HEALTH_DAMAGE_RECEIVED
Effect=casualties:-25%
