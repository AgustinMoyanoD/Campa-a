[MODIFIER]
Id=hades_inframundo
Name=Pacto del Inframundo
Description=Hades niega el descanso a tus bravos combatientes y entrega las riquezas del subsuelo. -60% muertes definitivas y +120 Monedas de oro de botin por victoria.
Kind=CARD
Source=HADES
Rarity=LEGENDARY
Tags=INFRAMUNDO, ORO
Duration=PERMANENTE

[RULE]
Hook=RECOUNT_DEATH
Effect=casualties:-60%

[RULE]
Hook=COMBAT_END
Effect=gold:+120
