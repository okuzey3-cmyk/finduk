import pickle, numpy as np
from PIL import Image, ImageDraw
from rend import render
A = pickle.load(open("assembled.pkl","rb"))
col = {"body":(236,232,224),"base":(150,156,166),"neck":(120,126,136),"head":(70,130,230),"plate_tft":(40,95,190)}
parts = [(A[k], col[k]) for k in col]
imgs = []
for az in (-35, 35, 180-35):
    imgs.append(render(parts, az, 14, size=(700,800), ppm=3.5, center=(0,0,95)))
canvas = Image.new("RGB",(2100,800),(245,245,245))
for i,im in enumerate(imgs): canvas.paste(im,(700*i,0))
canvas.save("onizleme.png")
# sadece kafa+plaka (oled) yakın plan, patlatılmış
