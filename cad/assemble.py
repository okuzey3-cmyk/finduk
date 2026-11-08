import pickle, numpy as np, trimesh
import finduk as f
P = pickle.load(open("parts.pkl","rb"))
GAP = 8.0
Zc = f.BH + f.NECK_H + GAP + f.HH/2
def T(rows, t):
    M = np.eye(4); M[:3,:3] = np.array(rows,float); M[:3,3] = t; return M
def ap(m, M):
    m = m.copy(); m.apply_transform(M); return m
M_head  = T([[1,0,0],[0,0,-1],[0,1,0]], [0, f.HD/2, Zc])
M_plate = T([[-1,0,0],[0,0,1],[0,1,0]], [0, -f.HD/2 - f.PLATE_T, Zc])
M_neck  = T(np.eye(3), [0,0,f.BH])
A = {
 "body": P["body"], "base": P["base"],
 "neck": ap(P["neck"], M_neck),
 "head": ap(P["head"], M_head),
 "plate_tft": ap(P["plate_tft"], M_plate),
 "plate_oled": ap(P["plate_oled"], M_plate),
}
pickle.dump(A, open("assembled.pkl","wb"))
# dummy electronics
def box(x0,x1,y0,y1,z0,z1): return f.bx(x0,x1,y0,y1,z0,z1)
esp = box(f.BRD_CX-f.BRD_L/2, f.BRD_CX+f.BRD_L/2, f.BRD_CY-f.BRD_W/2, f.BRD_CY+f.BRD_W/2, 4.0, 5.6)
amp = box(-f.AMP_L/2, f.AMP_L/2, -12-f.AMP_W/2, -12+f.AMP_W/2, 3.5, 5.1)
fy = -f.BD/2 + f.WALL
spk = f.bx(-f.SPK_L/2-0.3, f.SPK_L/2+0.3, fy+0.4, fy+0.4+f.SPK_D, f.SP_ZC-f.SPK_H/2, f.SP_ZC+f.SPK_H/2)
tft_local = box(-43.3,43.3,-25.3,25.3, f.HD-5, f.HD)     # head frame
tft = ap(tft_local, M_head)
servo_local = f.bx(-f.SHAFT_OFF-11.4, -f.SHAFT_OFF+11.4, -6.2, 6.2, f.NECK_H-15, f.NECK_H+0)
servo = ap(servo_local, M_neck)
def vol(a,b):
    try:
        i = trimesh.boolean.intersection([a,b], engine="manifold")
        return i.volume if len(i.vertices) else 0.0
    except Exception as e:
        return -1
pairs = [("body","base"),("body","neck"),("head","plate_tft"),("head","plate_oled"),("head","neck"),("neck","base")]
for a,b in pairs: print("çakışma", a, b, round(vol(A[a],A[b]),2))
print("ESP32 vs taban", round(vol(esp,A["base"]),2), "vs gövde", round(vol(esp,A["body"]),2))
print("Amfi vs taban", round(vol(amp,A["base"]),2), " vs gövde", round(vol(amp,A["body"]),2))
print("Hoparlör kutusu vs gövde", round(vol(spk,A["body"]),2), " vs taban", round(vol(spk,A["base"]),2), " vs amfi", round(vol(spk,amp),2), " vs esp", round(vol(spk,esp),2), " vs boyun", round(vol(spk,A["neck"]),2))
print("TFT kart vs kafa", round(vol(tft,A["head"]),2), " vs plaka-TFT", round(vol(tft,A["plate_tft"]),2))
print("Servo vs neck", round(vol(servo,A["neck"]),2))
