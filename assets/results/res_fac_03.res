[RESULT]
Id=res_fac_03_oro_clean
Name=Paso Comprado
Description=Los jinetes cuentan las monedas con una mueca salvaje. Golpean sus hachas contra los escudos y despejan el desfiladero.
Effect=gold:-25
Effect=moral:+2
Effect=faction_opinion:hierro:-5

[RESULT]
Id=res_fac_03_oro_burla
Name=Desprecio Bárbaro
Description=Aceptan el oro pero escupen al suelo, tachando a tu ejército de mercaderes cobardes. La moral de los sargentos se resiente.
Effect=gold:-25
Effect=moral:-6
Effect=faction_opinion:hierro:-15

[RESULT]
Id=res_fac_03_duelo_clean
Name=Victoria en la Arena de Montaña
Description=Un sargento veterano derriba al campeón bárbaro de un golpe de pomo. La Hermandad aclama la fuerza del ejército y ofrece jinetes para la marcha.
Effect=soldiers:+1
Effect=moral:+10
Effect=infamy:+4
Effect=faction_opinion:hierro:+20
Effect=set_flag:respeto_hierro

[RESULT]
Id=res_fac_03_duelo_herida
Name=Choque Sangriento
Description=El duelo es brutal. El campeón enemigo cae, pero tu campeón queda gravemente tullido. Los bárbaros se dispersan con aullidos de respeto.
Effect=soldiers:-1
Effect=salud:-8
Effect=moral:+4
Effect=infamy:+5
Effect=faction_opinion:hierro:+15
Effect=set_flag:respeto_hierro

[RESULT]
Id=res_fac_03_eval
Name=Encierro Táctico Brillante
Description=Tus tiradores ganan las cornisas antes de que los jinetes carguen. Atrapados en una bolsa mortal, los emisarios bajan las armas y ceden el paso.
Effect=moral:+8
Effect=infamy:+3
Effect=gold:+15
Effect=faction_opinion:hierro:+12
