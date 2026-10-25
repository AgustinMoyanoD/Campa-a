#!/usr/bin/env python3
"""
Genera el pack placeholder de UI en pixel art.

- Todo el arte "estructural" (marcos, paneles, barras) esta en tonos NEUTROS (grises):
  el color final sale del Tint que aplica el codigo (multiplicacion), asi un mismo marco
  sirve para dorado, verde, rojo o maldito.
- Los iconos son a color (identidad del recurso).
- Todo se dibuja a 1x. En el juego se escala con un factor ENTERO y filtro en punto.

Uso:  python3 generate_ui_pack.py        (requiere Pillow)
Escribe en ../assets/ui/, ../preview.png, ../README.md y ../ui_placeholder_pack.zip
"""
import os
import random
import shutil
from PIL import Image, ImageDraw, ImageFilter, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
UI = os.path.join(ROOT, "assets", "ui")
ICONS = os.path.join(UI, "icons")
os.makedirs(ICONS, exist_ok=True)


def g(v, a=255):
    return (int(v), int(v), int(v), int(a))


# ------------------------------------------------------------------
# Marcos con bisel (9-slice)
# ------------------------------------------------------------------
def make_frame(W, H, rings, fill, cut=True):
    """rings: lista de (claro, oscuro) desde el borde hacia adentro. Anillo 0 = contorno.
    Claro arriba/izquierda, oscuro abajo/derecha. Mas alla de los anillos: relleno plano."""
    im = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    px = im.load()
    for y in range(H):
        for x in range(W):
            r = min(x, y, W - 1 - x, H - 1 - y)
            if r < len(rings):
                lit_v, dark_v = rings[r]
                lit = (y == r and x < W - 1 - r) or (x == r and y < H - 1 - r)
                v = lit_v if lit else dark_v
            else:
                v = fill
            px[x, y] = g(max(0, min(255, v)))
    if cut:  # esquinas chaflanadas
        for (x, y) in [(0, 0), (W - 1, 0), (0, H - 1), (W - 1, H - 1)]:
            px[x, y] = (0, 0, 0, 0)
    return im


def button_cell(state):
    p = {
        "idle":     ([(22, 22), (255, 150), (215, 105), (68, 68)], 88),
        "hover":    ([(22, 22), (255, 175), (235, 125), (92, 92)], 112),
        "pressed":  ([(22, 22), (110, 235), (90, 190), (42, 42)], 62),
        "disabled": ([(22, 22), (150, 120), (135, 108), (52, 52)], 70),
    }[state]
    return make_frame(32, 32, p[0], p[1])


STUD = [
    [None, 40, 40, None],
    [40, 250, 210, 40],
    [40, 210, 160, 40],
    [None, 40, 40, None],
]


def add_studs(im, offset=2):
    W, H = im.size
    px = im.load()
    for ry, row in enumerate(STUD):
        for rx, v in enumerate(row):
            if v is None:
                continue
            for fx in (0, 1):
                for fy in (0, 1):
                    x = offset + rx if fx == 0 else W - 1 - (offset + rx)
                    y = offset + ry if fy == 0 else H - 1 - (offset + ry)
                    px[x, y] = g(v)


def panel_dark():
    im = make_frame(48, 48, [(18, 18), (245, 130), (215, 150), (185, 125), (60, 60), (35, 75)], 52)
    add_studs(im)
    return im


def panel_parchment():
    # marco "madera" (valores medios) + interior claro (pergamino al teñir en calido)
    im = make_frame(48, 48, [(18, 18), (205, 105), (165, 115), (135, 95), (50, 50), (188, 228)], 218)
    add_studs(im)
    return im


def slot_inset():
    return make_frame(24, 24, [(15, 15), (45, 170), (32, 95)], 28)


def bar_frame():
    return make_frame(24, 12, [(15, 15), (60, 150), (28, 28)], 22)


def bar_fill():
    im = Image.new("RGBA", (8, 8), (0, 0, 0, 0))
    px = im.load()
    grad = [255, 240, 225, 215, 205, 190, 170, 140]
    for y in range(8):
        for x in range(8):
            px[x, y] = g(grad[y])
    return im


def tooltip():
    return make_frame(32, 32, [(15, 15), (235, 165), (60, 60), (34, 34)], 24)


def chip():
    return make_frame(24, 16, [(15, 15), (235, 140), (80, 80)], 64)


# ------------------------------------------------------------------
# Superposiciones (overlays) para efectos
# ------------------------------------------------------------------
def highlight_overlay():
    """Brillo interno (se dibuja con mezcla aditiva sobre el boton)."""
    W = H = 32
    alphas = [(0, 0), (200, 60), (90, 20), (40, 10)]
    im = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    px = im.load()
    for y in range(H):
        for x in range(W):
            r = min(x, y, W - 1 - x, H - 1 - y)
            if r < len(alphas):
                lit = (y == r and x < W - 1 - r) or (x == r and y < H - 1 - r)
                a = alphas[r][0] if lit else alphas[r][1]
            else:
                a = 22
            px[x, y] = (255, 255, 255, a)
    return im


def glow_overlay():
    """Resplandor exterior (9-slice con borde 8, centro transparente; se dibuja expandido 8 px)."""
    W = H = 32
    steps = [0, 200, 150, 110, 75, 50, 30, 15, 0]
    im = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    px = im.load()
    for y in range(H):
        for x in range(W):
            dx = max(8 - x, x - 23, 0)
            dy = max(8 - y, y - 23, 0)
            d = max(dx, dy)  # distancia (Chebyshev) al rectangulo interior
            a = steps[min(d, 8)]
            px[x, y] = (255, 255, 255, a)
    return im


def crack_decal(stage):
    """Grietas (3 etapas, acumulativas). Decal de 32x32 para pegar en una esquina."""
    rnd = random.Random(1337)
    main = []
    x, y = 30, 1
    for _ in range(26):
        main.append((x, y))
        if rnd.random() < 0.85:
            x -= 1
        if rnd.random() < 0.85:
            y += 1
        if rnd.random() < 0.3:
            y += rnd.choice([-1, 0, 1])
        x = max(0, x)
        y = max(0, min(31, y))

    def branch(start, dx, dy, steps, seed):
        r = random.Random(seed)
        out = []
        bx, by = start
        for _ in range(steps):
            out.append((bx, by))
            if r.random() < 0.8:
                bx += dx
            if r.random() < 0.8:
                by += dy
            if r.random() < 0.3:
                by += r.choice([-1, 0, 1])
            bx = max(0, min(31, bx))
            by = max(0, min(31, by))
        return out

    count = {1: 10, 2: 18, 3: 26}[stage]
    pts = list(main[:count])
    if stage >= 2:
        pts += branch(main[7], 0, 1, 9, 11)
    if stage >= 3:
        pts += branch(main[13], -1, 0, 10, 23) + branch(main[18], 1, 1, 6, 5)

    im = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
    px = im.load()
    for (x, y) in pts:
        px[x, y] = (14, 14, 18, 255)
    for (x, y) in pts:  # reflejo claro 1 px abajo-derecha
        if x + 1 < 32 and y + 1 < 32 and px[x + 1, y + 1][3] == 0:
            px[x + 1, y + 1] = (255, 255, 255, 70)
    return im


# ------------------------------------------------------------------
# Iconos 16x16 (a color) con contorno automatico
# ------------------------------------------------------------------
def make_icon(draw_fn, outline=(20, 20, 30, 255)):
    shape = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
    draw_fn(ImageDraw.Draw(shape))
    alpha = shape.split()[3]
    dil = alpha.filter(ImageFilter.MaxFilter(3))
    base = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
    base.paste(Image.new("RGBA", (16, 16), outline), (0, 0), dil)
    base.alpha_composite(shape)
    return base


def d_agua(d):
    c = (70, 170, 240, 255)
    d.polygon([(8, 1), (4, 8), (12, 8)], fill=c)
    d.ellipse([3, 6, 12, 14], fill=c)
    d.point([(6, 9), (6, 10), (7, 11)], fill=(205, 238, 255, 255))


def d_vino(d):
    d.polygon([(4, 2), (11, 2), (10, 8), (5, 8)], fill=(150, 30, 60, 255))
    d.line([(5, 3), (10, 3)], fill=(225, 80, 110, 255))
    d.rectangle([7, 8, 8, 12], fill=(210, 210, 222, 255))
    d.rectangle([5, 13, 10, 14], fill=(210, 210, 222, 255))


def d_comida(d):
    d.ellipse([1, 5, 14, 13], fill=(214, 160, 80, 255))
    d.line([(5, 7), (6, 10)], fill=(160, 110, 50, 255))
    d.line([(8, 6), (9, 10)], fill=(160, 110, 50, 255))
    d.line([(11, 7), (12, 10)], fill=(160, 110, 50, 255))
    d.line([(4, 6), (11, 6)], fill=(238, 200, 125, 255))


def d_carne(d):
    d.ellipse([2, 2, 11, 10], fill=(190, 70, 60, 255))
    d.polygon([(9, 8), (13, 12), (12, 13), (8, 10)], fill=(240, 230, 210, 255))
    d.ellipse([12, 12, 14, 14], fill=(240, 230, 210, 255))
    d.ellipse([4, 3, 6, 5], fill=(232, 125, 112, 255))


def d_salud(d):
    c = (220, 55, 70, 255)
    d.ellipse([2, 3, 8, 9], fill=c)
    d.ellipse([7, 3, 13, 9], fill=c)
    d.polygon([(2, 7), (13, 7), (8, 14)], fill=c)
    d.point([(4, 5), (5, 5)], fill=(255, 170, 175, 255))


def d_moral(d):
    d.rectangle([3, 2, 4, 14], fill=(190, 170, 140, 255))
    d.polygon([(5, 3), (13, 5), (5, 9)], fill=(70, 120, 220, 255))
    d.line([(6, 5), (10, 5)], fill=(240, 210, 90, 255))


def d_satisfaccion(d):
    pts = [(8, 1), (10, 6), (15, 6), (11, 9), (13, 14), (8, 11), (3, 14), (5, 9), (1, 6), (6, 6)]
    d.polygon(pts, fill=(250, 205, 60, 255))


def d_ganado(d):
    d.polygon([(2, 3), (4, 5), (3, 6)], fill=(235, 225, 200, 255))
    d.polygon([(13, 3), (11, 5), (12, 6)], fill=(235, 225, 200, 255))
    d.rectangle([4, 4, 11, 10], fill=(235, 235, 235, 255))
    d.rectangle([5, 9, 10, 13], fill=(232, 165, 165, 255))
    d.point([(5, 6), (10, 6)], fill=(30, 30, 35, 255))
    d.point([(6, 11), (9, 11)], fill=(120, 60, 70, 255))


def d_aviso(d):
    d.polygon([(8, 1), (15, 14), (1, 14)], fill=(245, 200, 50, 255))
    d.rectangle([7, 6, 8, 10], fill=(30, 30, 35, 255))
    d.point([(7, 12), (8, 12)], fill=(30, 30, 35, 255))


def d_ok(d):
    d.line([(3, 8), (6, 12), (13, 3)], fill=(90, 210, 110, 255), width=2)


def d_peligro(d):
    d.line([(3, 3), (12, 12)], fill=(230, 70, 70, 255), width=2)
    d.line([(12, 3), (3, 12)], fill=(230, 70, 70, 255), width=2)


ICON_DEFS = [
    ("agua", d_agua), ("vino", d_vino), ("comida", d_comida), ("carne", d_carne),
    ("salud", d_salud), ("moral", d_moral), ("satisfaccion", d_satisfaccion),
    ("ganado", d_ganado), ("aviso", d_aviso), ("ok", d_ok), ("peligro", d_peligro),
]


# ------------------------------------------------------------------
# Vista previa (simula el 9-slice + tint; el juego usa DrawTextureNPatch)
# ------------------------------------------------------------------
def tinted(im, rgb):
    r, g_, b, a = im.split()
    r = r.point(lambda v: v * rgb[0] // 255)
    g_ = g_.point(lambda v: v * rgb[1] // 255)
    b = b.point(lambda v: v * rgb[2] // 255)
    return Image.merge("RGBA", (r, g_, b, a))


def nine_slice(cell, l, t, r, b, W, H):
    cw, ch = cell.size
    out = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    out.paste(cell.crop((0, 0, l, t)), (0, 0))
    out.paste(cell.crop((cw - r, 0, cw, t)), (W - r, 0))
    out.paste(cell.crop((0, ch - b, l, ch)), (0, H - b))
    out.paste(cell.crop((cw - r, ch - b, cw, ch)), (W - r, H - b))
    out.paste(cell.crop((l, 0, cw - r, t)).resize((W - l - r, t), Image.NEAREST), (l, 0))
    out.paste(cell.crop((l, ch - b, cw - r, ch)).resize((W - l - r, b), Image.NEAREST), (l, H - b))
    out.paste(cell.crop((0, t, l, ch - b)).resize((l, H - t - b), Image.NEAREST), (0, t))
    out.paste(cell.crop((cw - r, t, cw, ch - b)).resize((r, H - t - b), Image.NEAREST), (W - r, t))
    out.paste(cell.crop((l, t, cw - r, ch - b)).resize((W - l - r, H - t - b), Image.NEAREST), (l, t))
    return out


def build_preview(assets):
    font = ImageFont.load_default()
    CW, CH, SC = 396, 298, 3
    canvas = Image.new("RGBA", (CW, CH), (22, 26, 36, 255))
    d = ImageDraw.Draw(canvas)

    def put(img, x, y):
        canvas.alpha_composite(img, (x, y))

    def text(s, x, y, col=(235, 225, 200, 255)):
        d.text((x, y), s, fill=col, font=font)

    def button(x, y, w, h, state, tint, label, txtcol=(245, 235, 210, 255)):
        put(tinted(nine_slice(assets["btn"][state], 4, 4, 4, 4, w, h), tint), x, y)
        tw = d.textlength(label, font=font)
        text(label, int(x + (w - tw) / 2), y + h // 2 - 5 + (1 if state == "pressed" else 0), txtcol)

    GOLD = (225, 190, 120)
    text("UI placeholder pack (vista previa x3; 9-slice y tint simulados)", 6, 3)

    # fila A: estados del boton
    for i, (st, lab, tint) in enumerate([("idle", "Idle", GOLD), ("hover", "Hover", (245, 215, 150)),
                                         ("pressed", "Pressed", (190, 155, 95)),
                                         ("disabled", "Disabled", (130, 130, 130))]):
        col = (245, 235, 210, 255) if st != "disabled" else (140, 140, 140, 255)
        button(6 + i * 92, 16, 80, 22, st, tint, lab, col)

    # fila B: variantes de color, resplandor y brillo
    put(tinted(nine_slice(assets["glow"], 8, 8, 8, 8, 96, 38), (255, 220, 140)), 274, 46)
    button(6, 54, 80, 22, "idle", (120, 205, 140), "Confirmar")
    button(98, 54, 80, 22, "idle", (225, 105, 95), "Sacrificar")
    button(190, 54, 80, 22, "idle", (185, 115, 235), "Maldicion")
    button(282, 54, 80, 22, "idle", GOLD, "Elegido")
    put(nine_slice(assets["hl"], 4, 4, 4, 4, 80, 22), 282, 54)

    # fila C: grietas (decals en la esquina) y boton plano
    for i, stg in enumerate([1, 2, 3]):
        x = 6 + i * 92
        button(x, 92, 84, 24, "idle", GOLD, "Rotura %d" % stg)
        put(assets["crack"][stg], x + 84 - 32, 92 - 4)
    text("Texto plano", 292, 98, (200, 190, 160, 255))
    d.line([(292, 110), (350, 110)], fill=(120, 105, 75, 255))
    text("(sin marco)", 292, 114, (150, 140, 115, 255))

    # fila D: panel oscuro con barras y ranuras, panel pergamino con iconos
    put(tinted(nine_slice(assets["panel_dark"], 8, 8, 8, 8, 176, 76), (215, 185, 130)), 6, 126)
    for i, (pct, col) in enumerate([(0.80, (110, 200, 120)), (0.45, (230, 180, 70)), (0.15, (220, 85, 80))]):
        y = 138 + i * 16
        put(tinted(nine_slice(assets["bar_frame"], 3, 3, 3, 3, 112, 12), (200, 190, 170)), 16, y)
        fw = int((112 - 6) * pct)
        put(tinted(assets["bar_fill"].resize((max(1, fw), 6), Image.NEAREST), col), 19, y + 3)
    for i in range(2):
        put(tinted(nine_slice(assets["slot"], 3, 3, 3, 3, 28, 28), (190, 175, 150)), 142, 136 + i * 32)
    put(assets["icon"]["agua"], 148, 142)
    put(assets["icon"]["vino"], 148, 174)

    put(tinted(nine_slice(assets["panel_parch"], 8, 8, 8, 8, 176, 76), (255, 236, 206)), 194, 126)
    for i, n in enumerate(["comida", "carne", "salud", "moral", "satisfaccion", "ganado"]):
        put(assets["icon"][n], 208 + (i % 3) * 24, 140 + (i // 3) * 24)
    text("Pergamino", 290, 148, (60, 40, 20, 255))

    # fila E: tooltip y chips (texto claro sobre fondo oscuro tenido)
    put(tinted(nine_slice(assets["tooltip"], 4, 4, 4, 4, 160, 54), (230, 195, 120)), 6, 212)
    text("Tooltip", 16, 220, (255, 220, 120, 255))
    text("Texto explicativo corto.", 16, 234, (220, 220, 230, 255))
    x = 182
    for lab, tint in [("Completa", (110, 200, 120)), ("Reducida", (230, 180, 70)),
                      ("Hambrienta", (220, 85, 80))]:
        w = int(d.textlength(lab, font=font)) + 14
        put(tinted(nine_slice(assets["chip"], 3, 3, 3, 3, w, 16), tint), x, 214)
        text(lab, x + 7, 217, (240, 240, 232, 255))
        x += w + 6

    # fila F: iconos
    for i, (n, _) in enumerate(ICON_DEFS):
        put(assets["icon"][n], 6 + i * 20, 274)
    text("iconos 16x16", 236, 278, (180, 180, 190, 255))

    return canvas.resize((CW * SC, CH * SC), Image.NEAREST)


# ------------------------------------------------------------------
def save(im, *parts):
    path = os.path.join(*parts)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    im.save(path)


def main():
    # atlas de boton: 4 celdas horizontales (idle, hover, pressed, disabled)
    states = ["idle", "hover", "pressed", "disabled"]
    cells = {s: button_cell(s) for s in states}
    atlas = Image.new("RGBA", (32 * 4, 32), (0, 0, 0, 0))
    for i, s in enumerate(states):
        atlas.paste(cells[s], (32 * i, 0))
    save(atlas, UI, "ui_button.png")

    pd, pp = panel_dark(), panel_parchment()
    sl, bf, bfill = slot_inset(), bar_frame(), bar_fill()
    tt, ch = tooltip(), chip()
    hl, gl = highlight_overlay(), glow_overlay()
    cracks = {n: crack_decal(n) for n in (1, 2, 3)}
    for name, im in [("ui_panel_dark.png", pd), ("ui_panel_parchment.png", pp), ("ui_slot_inset.png", sl),
                     ("ui_bar_frame.png", bf), ("ui_bar_fill.png", bfill), ("ui_tooltip.png", tt),
                     ("ui_chip.png", ch), ("ui_highlight.png", hl), ("ui_glow.png", gl)]:
        save(im, UI, name)
    for n, im in cracks.items():
        save(im, UI, "ui_crack_%d.png" % n)
    icons = {}
    for name, fn in ICON_DEFS:
        icons[name] = make_icon(fn)
        save(icons[name], ICONS, "icon_%s.png" % name)

    assets = {"btn": cells, "panel_dark": pd, "panel_parch": pp, "slot": sl, "bar_frame": bf,
              "bar_fill": bfill, "tooltip": tt, "chip": ch, "hl": hl, "glow": gl,
              "crack": cracks, "icon": icons}
    build_preview(assets).convert("RGB").save(os.path.join(ROOT, "preview.png"))
    print("Pack generado en", ROOT)


if __name__ == "__main__":
    main()
