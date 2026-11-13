"""Hoparlör yerleşim önizlemesi: gövdenin üstü kesik, içi görünür (baskı için değil)."""
import pickle, numpy as np, trimesh
from PIL import Image, ImageDraw
import finduk_cift as f
from rend import render
A = pickle.load(open("assembled.pkl","rb"))
body, base = A["body"], A["base"]
kes = f.bx(-200,200,-200,200,62,300)
body_k = trimesh.boolean.difference([body,kes], engine="manifold")
fy = -f.BD/2 + f.WALL; fy2 = f.BD/2 - f.WALL
spk1 = f.bx(-f.SPK_L/2+0.5, f.SPK_L/2-0.5, fy+0.4, fy+0.4+f.SPK_D-1, f.SP_ZC-f.SPK_H/2+0.5, f.SP_ZC+f.SPK_H/2-0.5)
spk2 = f.bx(-f.SPK_L/2+0.5, f.SPK_L/2-0.5, fy2-0.4-f.SPK_D+1, fy2-0.4, f.SP_ZC-f.SPK_H/2+0.5, f.SP_ZC+f.SPK_H/2-0.5)
esp = f.bx(f.BRD_CX-f.BRD_L/2, f.BRD_CX+f.BRD_L/2, f.BRD_CY-f.BRD_W/2, f.BRD_CY+f.BRD_W/2, 4.0, 5.6)
amp = f.bx(-f.AMP_L/2, f.AMP_L/2, -12-f.AMP_W/2, -12+f.AMP_W/2, 3.5, 5.1)
on = trimesh.util.concatenate([body_k, base, spk1, spk2, esp, amp])
on.export("../stl_cift/7_yerlesim_onizleme_BASMA.stl")
parts=[(body_k,(236,232,224)),(base,(150,156,166)),(spk1,(40,40,48)),(spk2,(40,40,48)),(esp,(30,120,60)),(amp,(200,60,60))]
imgs=[render(parts,az,el,size=(800,700),ppm=4.6,center=(0,0,40)) for az,el in ((-30,38),(150,38),(0,85))]
cv=Image.new("RGB",(2400,760),(245,245,245))
for i,im in enumerate(imgs): cv.paste(im,(800*i,0))
d=ImageDraw.Draw(cv)
for i,t in enumerate(["Önden (üst kesik)","Arkadan","Üstten"]): d.text((800*i+20,715),t,fill=(40,40,40))
d.text((20,735),"Siyah: 2 hoparlör (100x40x20, ön ve arka duvar)  Yeşil: ESP32  Kırmızı: amfi",fill=(40,40,40))
cv.save("../stl_cift/yerlesim.png")
print("ok", on.extents)
