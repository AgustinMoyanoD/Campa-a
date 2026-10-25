# Pack placeholder de UI (pixel art)

Arte provisorio para probar el sistema de skins (marcos 9-slice, botones con estados, paneles, barras, efectos e iconos). Se generó con un script, así que **no contiene recursos de terceros** y no tiene problemas de licencia. Se reemplaza archivo por archivo cuando haya arte definitivo, manteniendo nombres y tamaños.

Copiá la carpeta `assets/ui/` dentro de `assets/` del proyecto. `preview.png` muestra cómo se ve (simulado con 9-slice y tint, a escala 3×).

## Contenido

| Archivo | Tamaño | Slice (izq, arr, der, abj) | Uso |
|---|---|---|---|
| `ui_button.png` | 128×32 (4 celdas de 32×32) | 4 4 4 4 | Botón: celdas idle, hover, pressed, disabled |
| `ui_panel_dark.png` | 48×48 | 8 8 8 8 | Panel oscuro con tachuelas en las esquinas |
| `ui_panel_parchment.png` | 48×48 | 8 8 8 8 | Panel con marco de madera e interior claro |
| `ui_slot_inset.png` | 24×24 | 3 3 3 3 | Ranura hundida (zona de soltar, ítems) |
| `ui_bar_frame.png` | 24×12 | 3 3 3 3 | Marco de barra |
| `ui_bar_fill.png` | 8×8 | sin slice | Relleno de barra (se estira en horizontal) |
| `ui_tooltip.png` | 32×32 | 4 4 4 4 | Fondo de tooltip |
| `ui_chip.png` | 24×16 | 3 3 3 3 | Etiqueta de estado |
| `ui_highlight.png` | 32×32 | 4 4 4 4 | Brillo interno, mezcla aditiva |
| `ui_glow.png` | 32×32 | 8 8 8 8 | Resplandor exterior (se dibuja expandido 8 px) |
| `ui_crack_1/2/3.png` | 32×32 | sin slice | Grietas en 3 etapas, se pegan en una esquina |
| `icons/icon_*.png` | 16×16 | sin slice | agua, vino, comida, carne, salud, moral, satisfacción, ganado, aviso, ok, peligro |
| `skins.cfg` | | | Propuesta de configuración de skins y estilos |

## Cómo usarlo

- **Color por tint.** El arte estructural está en grises neutros: el color final sale de multiplicar por un `Color` (tint) al dibujar. Un mismo marco sirve para dorado, verde, rojo o maldito. Los iconos son a color y se dibujan sin tint.
- **9-slice.** En Raylib: `DrawTextureNPatch` con un `NPatchInfo` por celda (los bordes de la tabla). Para el botón, el rectángulo fuente de cada estado es la celda correspondiente del atlas (x = 0, 32, 64, 96).
- **Pixel-perfect.** Cargá las texturas con `TEXTURE_FILTER_POINT`, escalá con un factor **entero** y redondeá los rectángulos de destino a enteros (si no, el arte tiembla o se ve borroso).
- **Efectos.**
  - *Highlight:* `ui_highlight.png` con `BLEND_ADDITIVE`; la intensidad es el alpha (hover, seleccionado).
  - *Glow:* `ui_glow.png` con `BLEND_ADDITIVE`, destino expandido 8 px × escala alrededor del elemento.
  - *Rotura:* decal de grietas según una etapa 0 a 3 (pegado en una esquina), más una sacudida con `tween`.
  - *Marco desactivable:* no dibujar el 9-slice y dejar solo el texto.
- **Texto sobre marcos.** Usá texto claro sobre fondos oscuros teñidos y texto oscuro solo sobre el pergamino.

## Regenerar o cambiar el estilo

`tools/generate_ui_pack.py` (requiere Pillow) regenera todo, incluida la vista previa. Los valores de bisel, colores base y tamaños están al principio de cada función, así que se puede ajustar sin dibujar a mano.

## Limitaciones

Es arte de relleno: marcos simples, sin ornamentos, sin variantes de esquina, e iconos mínimos. El centro de cada marco es plano porque el 9-slice lo estira; un patrón de fondo requiere dibujarlo aparte (por ejemplo en mosaico).
