[RESULT]
Id=res_urb_03_aislar_clean
Name=Ejército Limpio de Contagio
Description=Rodeas la plaza sin tocar sus muros. Se salvan todas las vidas y la moral se mantiene firme al esquivar la muerte pútrida.
Effect=salud:+4
Effect=moral:+4
Effect=set_flag:bloqueo_ciudad

[RESULT]
Id=res_urb_03_aislar_craso
Name=El Rodeo del Hambre
Description=El desvío por ciénagas cuesta un día entero de suministros. Craso gime al ver mermar las raciones sin conseguir reabastecimiento.
Effect=food:-15
Effect=salud:-4
Effect=moral:-4
Effect=loyalty:intendente:-2
Effect=set_flag:bloqueo_ciudad

[RESULT]
Id=res_urb_03_saquear_clean
Name=Granos Rescatados del Humo
Description=Tus veteranos entran con paños empapados en vinagre y sacan tres carretas de grano sellado. La audacia alimenta al ejército.
Effect=food:+35
Effect=moral:+4
Effect=faction_opinion:current:-10

[RESULT]
Id=res_urb_03_saquear_cost
Name=El Miasma Mortal
Description=El grano estaba contaminado. Dos soldados y un novato caen vomitando bilis negra antes de llegar a la puesta de sol.
Effect=initiates:-1
Effect=soldiers:-1
Effect=salud:-15
Effect=moral:-10

[RESULT]
Id=res_urb_03_eval
Name=Cordón Sanitario Magistral
Description=Identificas qué alhóndigas estaban a barlovento de la peste. Rescatas comida sana y Vitelio purifica a los hombres con hierbas amargas.
Effect=food:+25
Effect=salud:+6
Effect=loyalty:capellan:+3
Effect=moral:+6
Effect=faction_opinion:current:+15
