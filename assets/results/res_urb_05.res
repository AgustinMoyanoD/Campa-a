[RESULT]
Id=res_urb_05_rescate_clean
Name=Tributo Cobrado sin Bajas
Description=Carros de trigo y arquetas de oro cruzan el foso. El ejército se reabastece al máximo y marcha con los bolsillos llenos sin tocar las empalizadas.
Effect=gold:+50
Effect=food:+40
Effect=moral:+6
Effect=loyalty:intendente:+3
Effect=faction_opinion:current:+5

[RESULT]
Id=res_urb_05_rescate_cost
Name=Tregua Engañosa
Description=Entregan menos grano del prometido y usan la tregua para reforzar sus almenas. Varro lamenta no haber asaltado cuando estaban desprevenidos.
Effect=gold:+25
Effect=food:+15
Effect=loyalty:explorador:-3
Effect=moral:-4
Effect=set_flag:puertas_cerradas

[RESULT]
Id=res_urb_05_asalto_clean
Name=Murallas Conquistadas
Description=El ariete quiebra los goznes de roble. Tus soldados toman la plaza en una hora; el botín es inmenso y la fama marcial se propaga como un incendio.
Effect=gold:+80
Effect=food:+60
Effect=infamy:+15
Effect=moral:+12
Effect=faction_opinion:current:-30
Effect=faction_opinion:hierro:+15

[RESULT]
Id=res_urb_05_asalto_cost
Name=Brecha Sangrienta
Description=La guarnición arroja pez hirviendo y troncos con clavos. Tomas la plaza, pero la sangre de tus veteranos tiñe el foso de rojo.
Effect=soldiers:-1
Effect=initiates:-2
Effect=salud:-15
Effect=gold:+50
Effect=food:+40
Effect=infamy:+18
Effect=moral:-4
Effect=faction_opinion:current:-35

[RESULT]
Id=res_urb_05_lead
Name=Capitulación Total sin Romper una Lanza
Description=Tu renombre entre los guerreros del hierro convence al alcaide de que resistir es suicidio. Rinden la plaza y abren la armería sin disparar un virote.
Effect=gold:+60
Effect=food:+50
Effect=soldiers:+1
Effect=moral:+15
Effect=infamy:-5
Effect=faction_opinion:current:+20
