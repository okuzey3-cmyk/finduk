# Finduk sesini USB'den alip Mac hoparlorunden calar. Arduino Serial Monitor KAPALI olmali.
import glob, os, sys, termios, time, wave, subprocess, tempfile, select

portlar = glob.glob("/dev/cu.usbmodem*")
if not portlar:
    print("Robot bulunamadi. USB kablosunu kontrol et."); sys.exit(1)
fd = os.open(portlar[0], os.O_RDWR | os.O_NOCTTY)
a = termios.tcgetattr(fd)
a[0] = 0; a[1] = 0; a[2] = termios.CS8 | termios.CREAD | termios.CLOCAL; a[3] = 0
a[4] = a[5] = termios.B115200
termios.tcsetattr(fd, termios.TCSANOW, a)
print("Dinliyorum:", portlar[0], "(durdurmak icin Ctrl+C)")

tampon = bytearray(); ses = bytearray(); hiz = 24000; son = 0.0; metin = bytearray()

def cal():
    global ses
    if len(ses) < 4000:
        ses = bytearray(); return
    f = tempfile.NamedTemporaryFile(suffix=".wav", delete=False)
    w = wave.open(f.name, "wb"); w.setnchannels(1); w.setsampwidth(2); w.setframerate(hiz)
    w.writeframes(bytes(ses)); w.close()
    print("[ses caliniyor: %.1f sn]" % (len(ses) / 2 / hiz))
    subprocess.run(["afplay", f.name]); os.unlink(f.name); ses = bytearray()

while True:
    r, _, _ = select.select([fd], [], [], 0.3)
    if r:
        tampon += os.read(fd, 8192)
        while True:
            i = tampon.find(b"\xff\xaaPC")
            if i < 0:
                metin += tampon[:max(0, len(tampon) - 3)]; tampon = tampon[max(0, len(tampon) - 3):]; break
            metin += tampon[:i]
            if len(tampon) < i + 8: tampon = tampon[i:]; break
            n = tampon[i + 4] | (tampon[i + 5] << 8); hiz = tampon[i + 6] | (tampon[i + 7] << 8)
            if len(tampon) < i + 8 + n: tampon = tampon[i:]; break
            ses += tampon[i + 8:i + 8 + n]; tampon = tampon[i + 8 + n:]; son = time.time()
        while b"\n" in metin:
            satir, _, rest = bytes(metin).partition(b"\n"); metin = bytearray(rest)
            print(satir.decode("utf-8", "ignore").rstrip())
    if ses and time.time() - son > 1.2:
        cal()
