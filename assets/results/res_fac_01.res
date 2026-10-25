[RESULT]
Id=res_fac_01_comprar_clean
Name=Trato Comercial Honrado
Description=Pagas en oro acuñado. El factor del Gremio entrega grano fresco, vino y forraje, elogiando la probidad marcial de tu estandarte.
Effect=gold:-30
Effect=food:+35
Effect=water:+20
Effect=infamy:-5
Effect=moral:+4
Effect=faction_opinion:gremio:+10

[RESULT]
Id=res_fac_01_comprar_regateo
Name=Precios Inflados de Frontera
Description=El mercader detecta tu necesidad y eleva la tarifa. Consigues víveres pero el arca sufre y los sargentos mastican bronca.
Effect=gold:-30
Effect=food:+20
Effect=water:+15
Effect=moral:-2
Effect=faction_opinion:gremio:+5

[RESULT]
Id=res_fac_01_incautar_clean
Name=Escolta Ligera Dispersada
Description=Los pocos guardias de alquiler del Gremio huyen al ver tu columna. Requisas las mulas de grano sin combatir, mientras el carro blindado escapa al galope.
Effect=food:+45
Effect=water:+30
Effect=infamy:+12
Effect=moral:+2
Effect=faction_opinion:gremio:-20
Effect=set_flag:saqueo_gremio

[RESULT]
Id=res_fac_01_incautar_resistencia
Name=Virotes en la Retirada
Description=La reducida escolta dispara sus ballestas antes de huir. Tomas los sacos de trigo, pero dos reclutas caen heridos en las cunetas.
Effect=food:+35
Effect=water:+20
Effect=initiates:-2
Effect=infamy:+15
Effect=moral:-6
Effect=faction_opinion:gremio:-25
Effect=set_flag:saqueo_gremio

[RESULT]
Id=res_fac_01_asalto_tesoro
Name=Choque contra la Guardia del Tesoro
Description=Rodeas el carro blindado de caudales. El Gremio despliega a su numeroso destacamento de alabarderos veteranos para defender el oro hasta la muerte.
Effect=gold:+60
Effect=infamy:+18
Effect=faction_opinion:gremio:-35
Effect=set_flag:saqueo_gremio
Result_Battle=Guardia_Tesoro_Gremio
