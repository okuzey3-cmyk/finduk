import numpy as np
from PIL import Image, ImageDraw
def render(parts, azim, elev, size=(900,900), ppm=4.9, center=(0,30,80), explode=None):
    W_,H_ = size
    zbuf = np.full((H_,W_), -1e9); img = np.full((H_,W_,3), 245, np.uint8)
    th = np.radians(azim); ph = np.radians(elev)
    Rz = np.array([[np.cos(th),-np.sin(th),0],[np.sin(th),np.cos(th),0],[0,0,1]])
    Rx = np.array([[1,0,0],[0,np.cos(ph),-np.sin(ph)],[0,np.sin(ph),np.cos(ph)]])
    R = Rx @ Rz
    light = np.array([-0.35,-0.55,0.75]); light/=np.linalg.norm(light)
    c = np.array(center,float)
    for m,col in parts:
        V = (m.vertices - c) @ R.T
        F = m.faces
        tri = V[F]                     # (n,3,3) x, v, w (rotated)
        sx = tri[:,:,0]*ppm + W_/2
        sy = H_/2 - tri[:,:,2]*ppm
        dp = -tri[:,:,1]               # derinlik (buyuk = kameraya yakin)
        nrm = np.cross(tri[:,1]-tri[:,0], tri[:,2]-tri[:,0])
        ln = np.linalg.norm(nrm,axis=1); ln[ln==0]=1
        nrm = nrm/ln[:,None]
        # kameraya donuk normal: depth yonu = -v
        facing = -nrm[:,1]
        nrm = np.where((facing<0)[:,None], -nrm, nrm)
        L = R @ light  # isik dunya -> ekran
        shade = np.clip(nrm @ np.array([L[0],L[1],L[2]]),0,1)*0.65+0.35
        for i in range(len(F)):
            x0,y0 = sx[i,0],sy[i,0]; x1,y1 = sx[i,1],sy[i,1]; x2,y2 = sx[i,2],sy[i,2]
            minx=int(max(0,np.floor(min(x0,x1,x2)))); maxx=int(min(W_-1,np.ceil(max(x0,x1,x2))))
            miny=int(max(0,np.floor(min(y0,y1,y2)))); maxy=int(min(H_-1,np.ceil(max(y0,y1,y2))))
            if minx>maxx or miny>maxy: continue
            den = (y1-y2)*(x0-x2)+(x2-x1)*(y0-y2)
            if abs(den)<1e-9: continue
            xs,ys = np.meshgrid(np.arange(minx,maxx+1)+0.5, np.arange(miny,maxy+1)+0.5)
            a = ((y1-y2)*(xs-x2)+(x2-x1)*(ys-y2))/den
            b = ((y2-y0)*(xs-x2)+(x0-x2)*(ys-y2))/den
            g = 1-a-b
            inside = (a>=-1e-4)&(b>=-1e-4)&(g>=-1e-4)
            if not inside.any(): continue
            d = a*dp[i,0]+b*dp[i,1]+g*dp[i,2]
            sub = zbuf[miny:maxy+1, minx:maxx+1]
            upd = inside & (d>sub)
            sub[upd] = d[upd]
            img[miny:maxy+1, minx:maxx+1][upd] = (np.array(col)*shade[i]).astype(np.uint8)
    return Image.fromarray(img)

