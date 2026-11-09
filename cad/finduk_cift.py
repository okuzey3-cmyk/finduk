"""Finduk - modüler gövde (kafa / ön plaka / boyun / gövde / hoparlör paneli / taban)
Tüm ölçüler mm. Baskı yönünde STL üretir + birleşik önizleme için montaj konumlarını verir."""
import os, pickle
import numpy as np
import trimesh
from shapely.geometry import box as sbox

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "stl_cift")
os.makedirs(OUT, exist_ok=True)

# ---------------- ÖLÇÜLER ----------------
W = 100.0          # kafa ve gövde genişliği
RC = 14.0          # dış köşe yuvarlama
WALL = 2.4

# Kafa
HH = 70.0          # kafa yüksekliği
HD = 36.0          # kafa derinliği (ön plaka hariç)
PLATE_T = 3.0
POSTS_H = [(sx * 32.0, sy * 30.2) for sx in (-1, 1) for sy in (-1, 1)]
POST_R = 3.3
PILOT_R = 1.3      # M3 kendinden dişli vida için pilot delik
THRU_R = 1.7
PIVOT_Z = HD / 2   # servo ekseni kafa içinde
# TFT 2.8" (ILI9341) - mevcut ölçülerle
TFT_W, TFT_H = 86.6, 50.6
TFT_WIN_W, TFT_WIN_H = 58.0, 44.0
TFT_DX, TFT_DY = 0.0, 0.0
# OLED 0.96"
OLED_W, OLED_H = 27.3, 27.8
OLED_WIN_W, OLED_WIN_H = 24.5, 13.0
OLED_DY = -4.0     # kart merkezi, pencere merkezinin ne kadar altında
RING_H = 3.5

# Boyun
NECK_OD = 50.0
NECK_H = 32.0
FL_OD = 68.0
FL_T = 3.0
FL_R = 29.5 / np.sqrt(2)       # flanş vida delikleri (yarıçap 29.5)
SERVO_L, SERVO_W = 22.8, 12.4  # SG90 gövde
SERVO_EAR = 13.9               # kulak vidası ofseti (merkezden)
SHAFT_OFF = 5.2                # servo mili gövde merkezine göre kayık

# Gövde (5W kutulu hoparlör için genişletildi)
BW = 120.0         # gövde genişliği (kafa 100)
RCB = 12.0         # gövde dış köşe yuvarlama
BD = 80.0          # derinlik
BH = 88.0          # yükseklik
TOP_T = 3.0
BPOSTS = [(sx * 54.2, sy * 33.0) for sx in (-1, 1) for sy in (-1, 1)]
FRONT_POST_H = 19.0            # ön vida direkleri kısa (hoparlörün altında biter)
# Hoparlör: 8 ohm 5W kutulu, 100 x 45 x 21 mm
SPK_L, SPK_H, SPK_D = 100.0, 45.0, 21.0
SP_ZC = 44.0                   # hoparlör merkezinin yüksekliği
GR_L, GR_H = 88.0, 36.0        # ızgara bölgesi (uzun, kısa kenar)
GR_HOLE_R, GR_PITCH = 1.5, 4.4
WIN = 62.0
WIN_ZC = SP_ZC
SP_SCR = 33.0
BASE_T = 4.0
# Elektronik
BRD_L, BRD_W = 63.4, 27.1      # ESP32-S3 (ölçülmüş)
AMP_L, AMP_W = 18.8, 17.9      # MAX98357A (ölçülmüş)
BRD_CX, BRD_CY = -24.8, 14.0   # USB-C sol duvara yakın olsun diye sola kaydırıldı


# ---------------- YARDIMCI ----------------
def bx(x0, x1, y0, y1, z0, z1):
    m = trimesh.creation.box(extents=[x1 - x0, y1 - y0, z1 - z0])
    m.apply_translation([(x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2])
    return m


def rr(x0, x1, y0, y1, z0, z1, r):
    poly = sbox(x0 + r, y0 + r, x1 - r, y1 - r).buffer(r, resolution=16)
    m = trimesh.creation.extrude_polygon(poly, z1 - z0)
    m.apply_translation([0, 0, z0])
    return m


def cy(p0, p1, r, n=48):
    p0 = np.array(p0, float)
    p1 = np.array(p1, float)
    d = p1 - p0
    L = np.linalg.norm(d)
    m = trimesh.creation.cylinder(radius=r, height=L, sections=n)
    m.apply_transform(trimesh.geometry.align_vectors([0, 0, 1], d / L))
    m.apply_translation((p0 + p1) / 2)
    return m


def U(*ms):
    ms = [m for m in ms if m is not None]
    return ms[0] if len(ms) == 1 else trimesh.boolean.union(list(ms), engine="manifold")


def Dif(a, *bs):
    return trimesh.boolean.difference([a, U(*bs)], engine="manifold")


def save(m, name):
    m = m.copy()
    m.apply_translation([0, 0, -m.bounds[0][2]])
    m.merge_vertices()
    path = os.path.join(OUT, name)
    m.export(path)
    return m


# ---------------- KAFA (arka yüzü tablada) ----------------
def head():
    outer = rr(-W / 2, W / 2, -HH / 2, HH / 2, 0, HD, RC)
    cav = rr(-W / 2 + WALL, W / 2 - WALL, -HH / 2 + WALL, HH / 2 - WALL, WALL, HD + 1, RC - WALL)
    m = Dif(outer, cav)
    posts = [cy((x, y, 1.5), (x, y, HD), POST_R) for x, y in POSTS_H]
    # mikrofon halkası (üst duvar iç yüzünde) + ses deliği
    top_in = HH / 2 - WALL
    mic = Dif(cy((-14, top_in + 0.5, PIVOT_Z), (-14, top_in - 3.6, PIVOT_Z), 8.3),
              cy((-14, top_in + 1, PIVOT_Z), (-14, top_in - 4, PIVOT_Z), 6.7))
    m = U(m, *posts, mic)
    cuts = []
    for x, y in POSTS_H:
        cuts.append(cy((x, y, HD - 10), (x, y, HD + 1), PILOT_R))
    cuts.append(cy((-14, top_in - 1, PIVOT_Z), (-14, HH / 2 + 1, PIVOT_Z), 1.8))      # mikrofon deliği
    bot = -HH / 2
    for sx in (-10, 10):
        cuts.append(cy((sx, bot - 1, PIVOT_Z), (sx, bot + WALL + 1, PIVOT_Z), 1.15))  # servo kolu vidaları
    cuts.append(cy((0, bot - 1, PIVOT_Z), (0, bot + WALL + 1, PIVOT_Z), 3.5))         # orta vida erişimi
    cuts.append(bx(-7, 7, bot - 1, bot + WALL + 1, PIVOT_Z + 8, PIVOT_Z + 14))        # kablo yuvası
    return Dif(m, *cuts)


# ---------------- ÖN PLAKALAR (dış yüz tablada) ----------------
def plate_base():
    p = rr(-W / 2, W / 2, -HH / 2, HH / 2, 0, PLATE_T, RC)
    return p


def plate_holes():
    cuts = []
    for x, y in POSTS_H:
        cuts.append(cy((x, y, -1), (x, y, PLATE_T + 1), THRU_R))
        cuts.append(cy((x, y, -1), (x, y, 1.5), 3.0))          # vida başı yuvası
    return cuts


def plate_tft():
    p = plate_base()
    ring = Dif(rr(-TFT_W / 2 - 0.2 - 1.2, TFT_W / 2 + 0.2 + 1.2, -TFT_H / 2 - 0.2 - 1.2, TFT_H / 2 + 0.2 + 1.2,
                  PLATE_T - 0.2, PLATE_T + RING_H, 2.0),
               rr(-TFT_W / 2 - 0.2, TFT_W / 2 + 0.2, -TFT_H / 2 - 0.2, TFT_H / 2 + 0.2, PLATE_T - 1, PLATE_T + RING_H + 1, 1.0))
    m = U(p, ring)
    win = rr(-TFT_WIN_W / 2 + TFT_DX, TFT_WIN_W / 2 + TFT_DX, -TFT_WIN_H / 2 + TFT_DY, TFT_WIN_H / 2 + TFT_DY, -1, PLATE_T + 1, 1.0)
    return Dif(m, win, *plate_holes())


def plate_oled():
    p = plate_base()
    cy_ = OLED_DY
    iw, ih = OLED_W + 0.6, OLED_H + 0.6
    ring = Dif(rr(-iw / 2 - 1.2, iw / 2 + 1.2, cy_ - ih / 2 - 1.2, cy_ + ih / 2 + 1.2, PLATE_T - 0.2, PLATE_T + RING_H, 2.0),
               rr(-iw / 2, iw / 2, cy_ - ih / 2, cy_ + ih / 2, PLATE_T - 1, PLATE_T + RING_H + 1, 0.8),
               bx(-7, 7, cy_ - ih / 2 - 3, cy_ - ih / 2 + 3, PLATE_T - 1, PLATE_T + RING_H + 1))   # pin/kablo çıkışı
    m = U(p, ring)
    win = rr(-OLED_WIN_W / 2, OLED_WIN_W / 2, -OLED_WIN_H / 2, OLED_WIN_H / 2, -1, PLATE_T + 1, 1.0)
    return Dif(m, win, *plate_holes())


# ---------------- BOYUN (flanş tablada) ----------------
def neck():
    fl = cy((0, 0, 0), (0, 0, FL_T), FL_OD / 2, n=96)
    tube = cy((0, 0, 0), (0, 0, NECK_H), NECK_OD / 2, n=96)
    m = U(fl, tube)
    bore = cy((0, 0, -1), (0, 0, NECK_H - 3.0), NECK_OD / 2 - WALL, n=96)
    cuts = [bore]
    for sx in (-1, 1):
        for sy in (-1, 1):
            x, y = sx * FL_R, sy * FL_R
            cuts.append(cy((x, y, -1), (x, y, FL_T + 1), THRU_R))
    # servo yuvası (üst plaka)
    cx = -SHAFT_OFF
    cuts.append(bx(cx - (SERVO_L + 0.4) / 2, cx + (SERVO_L + 0.4) / 2, -(SERVO_W + 0.4) / 2, (SERVO_W + 0.4) / 2, NECK_H - 4, NECK_H + 1))
    for sx in (-1, 1):
        x = cx + sx * SERVO_EAR
        cuts.append(cy((x, 0, NECK_H - 3), (x, 0, NECK_H + 1), 0.95))
    cuts.append(bx(-6, 6, 12.5, 17.5, NECK_H - 4, NECK_H + 1))   # kafa kablosu
    return Dif(m, *cuts)


# ---------------- GÖVDE (montaj yönünde; baskıda ters çevrilir) ----------------
def grille_centers():
    """Ön duvardaki ızgara delik merkezleri: petek dizilim, uzun yuvarlak uçlu (stadyum) bölge içinde."""
    pts = []
    cap_r = GR_H / 2
    cx_end = GR_L / 2 - cap_r
    margin = GR_HOLE_R + 1.0
    dy = GR_PITCH * np.sqrt(3) / 2
    nrow = int((GR_H / 2) / dy) + 1
    for j in range(-nrow, nrow + 1):
        z = j * dy
        off = (GR_PITCH / 2) if (j % 2) else 0.0
        for i in range(-30, 31):
            x = i * GR_PITCH + off
            if abs(z) > cap_r - margin:
                continue
            if abs(x) <= cx_end:
                ok = True
            else:
                ok = np.hypot(abs(x) - cx_end, z) <= cap_r - margin
            if ok:
                pts.append((x, z))
    return pts


def body():
    outer = rr(-BW / 2, BW / 2, -BD / 2, BD / 2, 0, BH, RCB)
    cav = rr(-BW / 2 + WALL, BW / 2 - WALL, -BD / 2 + WALL, BD / 2 - WALL, -1, BH - TOP_T, RCB - WALL)
    m = Dif(outer, cav)
    add = []
    for x, y in BPOSTS:
        h = (BH - TOP_T + 0.5) if y > 0 else FRONT_POST_H
        add.append(cy((x, y, 0), (x, y, h), POST_R))
    for sx in (-1, 1):
        for sy in (-1, 1):
            add.append(cy((sx * FL_R, sy * FL_R, BH - TOP_T - 10), (sx * FL_R, sy * FL_R, BH - TOP_T + 0.5), 3.5))
    # hoparlörü altından taşıyan raf (5 mm çıkıntı, baskıda destek istemez)
    fy = -BD / 2 + WALL
    zb = SP_ZC - SPK_H / 2 - 0.2
    add.append(bx(-40, 40, fy - 0.5, fy + 5.0, zb - 2.5, zb))
    fy2 = BD / 2 - WALL                       # arka duvar (ikinci hoparlor)
    add.append(bx(-40, 40, fy2 - 5.0, fy2 + 0.5, zb - 2.5, zb))
    m = U(m, *add)
    cuts = []
    for x, y in BPOSTS:
        cuts.append(cy((x, y, -1), (x, y, 14), PILOT_R))
    for sx in (-1, 1):
        for sy in (-1, 1):
            cuts.append(cy((sx * FL_R, sy * FL_R, BH - TOP_T - 10.5), (sx * FL_R, sy * FL_R, BH + 1), PILOT_R))
    cuts.append(cy((0, 0, BH - TOP_T - 1), (0, 0, BH + 1), 19.0, n=64))                       # kablo geçişi
    for x, dz in grille_centers():                                                             # hoparlör ızgarası
        cuts.append(cy((x, -BD / 2 - 1, SP_ZC + dz), (x, fy + 0.6, SP_ZC + dz), GR_HOLE_R, n=14))
    for x, dz in grille_centers():                                                             # arka hoparlor izgarasi
        cuts.append(cy((x, fy2 - 0.6, SP_ZC + dz), (x, BD / 2 + 1, SP_ZC + dz), GR_HOLE_R, n=14))
    cuts.append(bx(-BW / 2 - 1, -BW / 2 + WALL + 1, BRD_CY - 6, BRD_CY + 6, 3.7, 10.7))        # USB-C
    return Dif(m, *cuts)


# ---------------- HOPARLÖR PANELİ (dış yüz tablada) ----------------
def speaker_panel(S):
    P = 70.0
    p = rr(-P / 2, P / 2, -P / 2, P / 2, 0, PLATE_T, 5.0)
    ring = Dif(cy((0, 0, PLATE_T - 0.2), (0, 0, PLATE_T + 4.0), S / 2 + 1.6, n=96),
               cy((0, 0, PLATE_T - 1), (0, 0, PLATE_T + 5), S / 2 + 0.2, n=96))
    m = U(p, ring)
    cuts = []
    for sx in (-1, 1):
        for sy in (-1, 1):
            x, y = sx * SP_SCR, sy * SP_SCR
            cuts.append(cy((x, y, -1), (x, y, PLATE_T + 1), THRU_R))
            cuts.append(cy((x, y, -1), (x, y, 1.5), 3.0))
    # ızgara delikleri
    cuts.append(cy((0, 0, -1), (0, 0, PLATE_T + 1), 1.3, n=16))
    rmax = (S - 9) / 2
    k = 1
    while k * 4.6 <= rmax:
        r = k * 4.6
        n = int(2 * np.pi * r / 4.8)
        for i in range(n):
            a = 2 * np.pi * i / n
            cuts.append(cy((r * np.cos(a), r * np.sin(a), -1), (r * np.cos(a), r * np.sin(a), PLATE_T + 1), 1.3, n=16))
        k += 1
    return Dif(m, *cuts)


# ---------------- TABAN (alt yüzü tablada) ----------------
def base():
    plate = rr(-BW / 2, BW / 2, -BD / 2, BD / 2, -BASE_T, 0, RCB)
    ro = RCB - WALL - 0.25
    spig = Dif(rr(-BW / 2 + WALL + 0.25, BW / 2 - WALL - 0.25, -BD / 2 + WALL + 0.25, BD / 2 - WALL - 0.25, -0.2, 2.5, ro),
               rr(-BW / 2 + WALL + 1.85, BW / 2 - WALL - 1.85, -BD / 2 + WALL + 1.85, BD / 2 - WALL - 1.85, -1, 3.5, ro - 1.6))
    notch = [cy((x, y, -1), (x, y, 3.5), POST_R + 0.8) for x, y in BPOSTS]
    spig = Dif(spig, *notch)
    # ESP32 rayları
    b0, b1 = BRD_CY - BRD_W / 2, BRD_CY + BRD_W / 2
    xa, xb = BRD_CX - BRD_L / 2 + 9, BRD_CX + BRD_L / 2 - 1
    pr = []
    pr.append(bx(xa, xb, b0 - 0.3 - 1.2, b0 - 0.3, -0.2, 6.8))
    pr.append(bx(xa, xb, b1 + 0.3, b1 + 0.3 + 1.2, -0.2, 6.8))
    pr.append(bx(xa, xb, b0 - 0.3, b0 + 2.7, -0.2, 4.0))
    pr.append(bx(xa, xb, b1 - 2.7, b1 + 0.3, -0.2, 4.0))
    pr.append(bx(BRD_CX + BRD_L / 2 + 0.3, BRD_CX + BRD_L / 2 + 1.5, BRD_CY - 6, BRD_CY + 6, -0.2, 6.8))
    # amfi yuvası
    ax, ay = 0.0, -12.0
    pr.append(bx(ax - AMP_L / 2 - 1.2, ax - AMP_L / 2 - 0.1, ay - 4, ay + 4, -0.2, 7.0))
    pr.append(bx(ax + AMP_L / 2 + 0.1, ax + AMP_L / 2 + 1.2, ay - 4, ay + 4, -0.2, 7.0))
    for sx in (-1, 1):
        for sy in (-1, 1):
            pr.append(bx(ax + sx * (AMP_L / 2 - 2.0) - 2.0, ax + sx * (AMP_L / 2 - 2.0) + 2.0,
                         ay + sy * (AMP_W / 2 - 2.0) - 2.0, ay + sy * (AMP_W / 2 - 2.0) + 2.0, -0.2, 3.5))
    m = U(plate, spig, *pr)
    cuts = []
    for x, y in BPOSTS:
        cuts.append(cy((x, y, -BASE_T - 1), (x, y, 1), THRU_R))
        cuts.append(cy((x, y, -BASE_T - 1), (x, y, -BASE_T + 1.5), 3.1))
    return Dif(m, *cuts)


# ---------------- ÜRET ----------------
def flip_for_print(m):
    m = m.copy()
    m.apply_transform(trimesh.transformations.rotation_matrix(np.pi, [1, 0, 0]))
    return m


if __name__ == "__main__":
    P = {}
    P["head"] = head()
    P["plate_tft"] = plate_tft()
    P["plate_oled"] = plate_oled()
    P["neck"] = neck()
    P["body"] = body()
    P["base"] = base()
    names = {
        "head": "1_kafa.stl", "plate_tft": "2a_on_plaka_TFT.stl", "plate_oled": "2b_on_plaka_OLED.stl",
        "neck": "3_boyun_MG90S.stl", "body": "4_govde_cift_hoparlor.stl", "base": "6_taban.stl",
    }
    for k, n in names.items():
        m = P[k]
        out = flip_for_print(m) if k == "body" else m
        s = save(out, n)
        print(n, "watertight" if s.is_watertight else "NOT WATERTIGHT", np.round(s.extents, 1))
    pickle.dump(P, open(os.path.join(os.path.dirname(OUT), "parts.pkl"), "wb"))
