[MODIFIER]
Id=curse_weakness
Name=Fatiga de los Titanes
Description=Un agotamiento plomizo atenaza los brazos de tus guerreros. Reduce un 30% el dano de choque infligido al enemigo durante 3 batallas.
Kind=CURSE
Source=TARTARO
Rarity=RARE
Target=ARMY
Tags=MALDICION, DEBILIDAD
Duration=BATTLES:3

[RULE]
Hook=EXEC_DAMAGE
Effect=execution:-30%
