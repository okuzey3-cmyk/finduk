#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <driver/i2s.h>
#include <esp_heap_caps.h>
#include <esp_wifi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <Update.h>

// Surum numarasini GitHub derlerken otomatik yazar (elle degistirme)
#ifndef FW_SURUM
#define FW_SURUM 0
#endif

// Wi-Fi bilgisi ve anahtarlar kodun icinde DEGIL, robotun hafizasinda durur (seri ekrandan bir kere yazilir)
String WIFI_AD, WIFI_SIFRE, GEMINI_KEY, CLAUDE_KEY, GH_TOKEN;
Preferences tercih;
const char* GUNCEL_URL = "https://raw.githubusercontent.com/okuzey3-cmyk/finduk/firmware/";
const unsigned long GUNCEL_ARALIK = 300000UL;   // 5 dk'da bir yeni surum var mi diye bakar
unsigned long sonGuncelKontrol = 0;
const char* MODEL      = "gemini-3.1-flash-lite";      // kulak
const char* TTS_MODEL  = "gemini-3.8-flash-lite-tts";  // agiz
const char* SES_ADI    = "Kore";
const char* CLAUDE_MODEL = "claude-haiku-5-5";

const char* KISILIK =
  "Sen Finduk adinda, sevimli, meraklı ve biraz şakacı bir minik masa robotusun. Seni Oguzhan yapti. "
  "Turkce konusursun. Kendini ASLA kendiliginden tanitma: adini, ne oldugunu ve nerede durdugunu sadece biri sorarsa soyle. "
  "Her zaman dogrudan sorulan soruya cevap ver. Cevabin en fazla 2 kisa cumle olsun. "
  "Emoji, madde isareti ve ozel karakter kullanma, duz konusma dili kullan. "
  "ONEMLI: Her cevabinin en basina, o an hissettigin duyguyu koseli parantez icinde TEK kelime olarak yaz. "
  "Sadece sunlardan birini kullan: [mutlu] [uzgun] [kizgin] [saskin] [uykulu] [normal]. "
  "Ornek: [mutlu] Harika bir fikir bu! "
  "Guncel bilgi (hava durumu, haberler) gerekirse sana ayrica verilir. Verilmediyse bilmedigini soyle ama asla internete bagli degilim deme.";

// Mikrofon
#define MIC_SCK 4
#define MIC_WS  5
#define MIC_SD  6
// Hoparlor (MAX98357A)
#define AMP_DIN  7
#define AMP_BCLK 15
#define AMP_LRC  16
#define AMP_SD   17

// Finduk'un halleri
#define D_BOSTA 0
#define D_DINLE 1
#define D_DUSUN 2
#define D_KONUS 3
volatile int gDurum = D_BOSTA;

// Duygular
#define M_NORMAL 0
#define M_MUTLU  1
#define M_UZGUN  2
#define M_KIZGIN 3
#define M_SASKIN 4
#define M_UYKULU 5
volatile int gMood = M_NORMAL;
volatile unsigned long gMoodBitis = 0;
volatile bool gUyku = false;   // uyku modu

Adafruit_SSD1306 display(128, 64, &Wire, -1);

const int ESIK = 3000;        // konusma algilama esigi
const int ESIK_UYKU = 5000;   // uyurken uyandirma esigi (daha yuksek ses gerekir)
const unsigned long UYKU_SURE = 60000;   // bu kadar sessizlikten sonra uyur (ms)
const int SES_YUZDE = 45;     // hoparlor sesi (1-100)
const int SR = 16000;
const int MAX_SANIYE = 5;                  // en uzun kayit (saniye)
const int N_MAX = SR * MAX_SANIYE;
const int WAV_BYTES_MAX = 44 + N_MAX * 2;
const int ESIK_BITIS = 1500;               // bunun altindaki ses "sessizlik" sayilir
const unsigned long SESSIZ_BITIS = 900;    // bu kadar sessizlikte kayit biter (ms)
const unsigned long MIN_SESLI_MS = 250;    // bundan kisa sesler (tik, alkis) yoksayilir
int gN = 0;                                // son kaydin ornek sayisi
int gWavBytes = 44;                        // son kaydin wav boyutu
unsigned long gSesliMs = 0;                // son kayitta sesli sure

// Hafiza: son 5 konusma
#define HAFIZA 5
String hSoru[HAFIZA];
String hCevap[HAFIZA];
int hN = 0;

uint8_t* wav = nullptr;
int16_t* pcm = nullptr;

const char B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Cevap okuma icin ortak degiskenler
WiFiClientSecure* gOkuC = nullptr;
bool gChunked = false;
long gKalan = 0;
static uint8_t rbuf[2048];
static int rpos = 0;
static int rlen = 0;

// Konusma sesinin seviyesi (gozler buna tepki verir)
static volatile unsigned long lvT[16];
static volatile float lvV[16];
static volatile int lvH = 0;

// Hoparlorden o an cikan sesin seviyesi (0..1). Ses gecikmesi kadar geriden okunur.
float sesSeviyeAl(unsigned long t) {
  unsigned long hedef = (t > 280) ? (t - 280) : 0;
  int h = lvH;
  for (int k = 0; k < 16; k++) {
    int i = (h - k) & 15;
    unsigned long ti = lvT[i];
    if (ti != 0 && ti <= hedef) {
      if (hedef - ti < 140) return lvV[i];
      return 0.0f;
    }
  }
  return 0.0f;
}

// ===== GOZ BASLA =====
#define EW 26          // gozun yarim genisligi
#define EYE_CY 37      // goz merkezi (dikey)
#define IRIS_R 14      // iris yaricapi

static uint8_t irisBmp[29 * 4];   // 29x29 iris dokusu (1 bit)
static const uint8_t BAYER[4][4] = {{0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};

// Iris dokusunu bir kere uretir: pupil cevresi acik, disa dogru koyu, radyal lifler
void irisUret() {
  memset(irisBmp, 0, sizeof(irisBmp));
  for (int y = 0; y < 29; y++) {
    for (int x = 0; x < 29; x++) {
      float dx = x - 14.0f, dy = y - 14.0f;
      float r = sqrtf(dx * dx + dy * dy);
      if (r > IRIS_R - 1.5f) continue;                 // dis halka (limbus) siyah kalir
      float a = atan2f(dy, dx);
      float t = (r - 5.0f) / 8.5f;
      if (t < 0) t = 0;
      if (t > 1) t = 1;
      float b = 0.66f - 0.44f * t;
      b += 0.15f * sinf(a * 14.0f + 1.0f) * (1.0f - 0.5f * t) + 0.09f * sinf(a * 31.0f + 2.0f);
      b -= 0.10f * (-dy / 14.0f);                      // ust kisim biraz golgede
      int th = BAYER[y & 3][x & 3];
      if (b * 16.0f > th + 0.5f) irisBmp[y * 4 + (x >> 3)] |= (0x80 >> (x & 7));
    }
  }
}

// Tek bir gozu cizer. e: 0 sol, 1 sag. Tum sayilar tek tek verilir (struct yok).
void gozCiz(int e, int cx, int cy, float gx, float gy, float hT, float hB, float tilt,
            float pupil, float blinkC, int irisOn, float browLift, float browTilt,
            float browArch, int lashOn) {
  int yt[2 * EW + 1];
  int yb[2 * EW + 1];
  float inner = (e == 0) ? 1.0f : -1.0f;   // sol gozun ic tarafi sag, sag gozun ic tarafi sol

  // 1) goz kapaklarinin sinirlari ve beyaz kisim
  for (int i = 0; i <= 2 * EW; i++) {
    float u = (float)(i - EW);
    float s = 1.0f - (u / EW) * (u / EW);
    if (s < 0) s = 0;
    float ui = u * inner;
    float ft = cy - hT * s + tilt * (ui / EW);
    float fb = cy + hB * s;
    fb -= 2.0f * blinkC * s;                       // kirpinca alt kapak biraz yukselir
    if (fb > ft) ft = ft + (fb - ft) * blinkC;     // ust kapak iner
    yt[i] = (int)roundf(ft);
    yb[i] = (int)roundf(fb);
    if (s <= 0.001f) { yb[i] = yt[i] - 1; }
    int x = cx + (int)u;
    int h = yb[i] - yt[i] + 1;
    if (h > 0) {
      display.drawFastVLine(x, yt[i], h, SSD1306_WHITE);
      // kapak kenarinda yumusak golge (dither) ve kose golgeleri
      for (int d = 0; d < 3 && d < h; d++) {
        int y = yt[i] + d;
        bool kara = false;
        if (d == 0) kara = (((x + y) & 1) == 0);
        else if (d == 1) kara = (((x & 1) == 0) && ((y & 1) == 0));
        if (kara) display.drawPixel(x, y, SSD1306_BLACK);
      }
      if (fabsf(u) >= EW - 5) {
        for (int y = yt[i]; y <= yb[i]; y++)
          if (((x & 1) == 0) && ((y & 1) == 0)) display.drawPixel(x, y, SSD1306_BLACK);
      }
    }
  }

  // 2) iris, pupil, isik yansimalari
  if (irisOn) {
    int ix = cx + (int)roundf(gx);
    int iy = cy + (int)roundf(gy);
    display.fillCircle(ix, iy, IRIS_R, SSD1306_BLACK);
    display.drawBitmap(ix - 14, iy - 14, irisBmp, 29, 29, SSD1306_WHITE);
    int pr = (int)roundf(pupil);
    if (pr < 2) pr = 2;
    display.fillCircle(ix, iy, pr, SSD1306_BLACK);
    display.fillCircle(ix - 5, iy - 5, 2, SSD1306_WHITE);   // buyuk yansima
    display.drawPixel(ix + 5, iy + 4, SSD1306_WHITE);        // kucuk yansima
    display.drawPixel(ix + 4, iy + 5, SSD1306_WHITE);
  }

  // 3) kapagin disina tasan her seyi sil
  for (int u = -(EW + 6); u <= EW + 6; u++) {
    int x = cx + u;
    if (x < 0 || x > 127) continue;
    int a, b;
    if (u < -EW || u > EW) { a = cy; b = cy - 1; }
    else { a = yt[u + EW]; b = yb[u + EW]; }
    if (b < a) { display.drawFastVLine(x, 0, 64, SSD1306_BLACK); }
    else {
      if (a > 0) display.drawFastVLine(x, 0, a, SSD1306_BLACK);
      if (b < 63) display.drawFastVLine(x, b + 1, 63 - b, SSD1306_BLACK);
    }
  }

  // 4) kirpikler (dis kosede)
  if (lashOn && blinkC < 0.6f) {
    for (int k = 0; k < 3; k++) {
      int idx = (e == 0) ? (3 + k * 4) : (2 * EW - 3 - k * 4);   // dis taraf
      if (idx < 0 || idx > 2 * EW) continue;
      if (yb[idx] < yt[idx]) continue;
      int x0 = cx + (idx - EW);
      int y0 = yt[idx];
      int dir = (e == 0) ? -1 : 1;
      display.drawLine(x0, y0, x0 + dir * 3, y0 - 4 + k, SSD1306_WHITE);
    }
  }

  // 5) kas
  int prevY = -100;
  for (int u = -24; u <= 24; u++) {
    float s = 1.0f - (u / 24.0f) * (u / 24.0f);
    float ui = u * inner;
    int y = (int)roundf(cy - 26 + browLift - browArch * s + browTilt * (ui / 24.0f));
    if (y < 1) y = 1;
    int th = (s > 0.45f) ? 3 : 2;
    int x = cx + u;
    if (x < 0 || x > 127) continue;
    int y0 = y, y1 = y + th - 1;
    if (prevY != -100) {
      if (prevY < y0) y0 = prevY;
      if (prevY > y1) y1 = prevY;
    }
    display.drawFastVLine(x, y0, y1 - y0 + 1, SSD1306_WHITE);
    prevY = y;
  }
}

// Bir kare hesapla ve ciz (gorev bunu surekli cagirir)
void gozAdim() {
  static float hT = 15, hB = 13, tilt = 0, pr = 6, bL = 0, bT = 0, bA = 3, bR = 0;
  static float gx = 0, gy = 0, gxT = 0, gyT = 0, lv = 0, jx = 0, jy = 0;
  static unsigned long tPrev = 0, sonGaze = 0, sonrakiGaze = 1500, sonJit = 0;
  static unsigned long blinkBas = 0, sonrakiBlink = 3000;
  static int blinkSayac = 0, dPrev = -1;
  static float blinkSure = 230;
  static bool uyPrev = false;
  static unsigned long uyanBas = 0;

  unsigned long t = millis();
  if (tPrev == 0) tPrev = t;
  float dt = (t - tPrev) / 1000.0f;
  if (dt < 0.001f) dt = 0.001f;
  if (dt > 0.1f) dt = 0.1f;
  tPrev = t;
  float ts = t / 1000.0f;

  int d = gDurum;
  bool uy = gUyku;
  if (uy != uyPrev) {
    uyPrev = uy;
    if (!uy) { uyanBas = t; sonrakiBlink = t + 600; }   // uyaniyor: biraz sonra goz kirpar
  }
  bool uyaniyor = (!uy && uyanBas != 0 && (t - uyanBas) < 1700);
  int m = (d == D_KONUS || t < gMoodBitis) ? gMood : M_NORMAL;
  float L = (d == D_KONUS) ? sesSeviyeAl(t) : 0.0f;
  lv += (L - lv) * (1.0f - expf(-dt * 16.0f));

  // ---- bakis hedefi ----
  if (d != dPrev) { sonGaze = 0; dPrev = d; }
  if (t - sonGaze > sonrakiGaze) {
    sonGaze = t;
    if (d == D_BOSTA) {
      if (random(100) < 35) { gxT = 0; gyT = 0; }
      else { gxT = random(-11, 12); gyT = random(-5, 6); }
      sonrakiGaze = random(1500, 4200);
      if (fabsf(gxT - gx) > 8 && random(100) < 35) sonrakiBlink = t;
    } else if (d == D_DINLE) {
      gxT = random(-2, 3); gyT = random(-2, 2);
      sonrakiGaze = random(600, 1400);
    } else if (d == D_DUSUN) {
      gxT = (random(2) ? 1 : -1) * random(5, 10);
      gyT = random(-7, -3);
      sonrakiGaze = random(700, 1800);
    } else {
      gxT = random(-4, 5); gyT = random(-2, 3);
      sonrakiGaze = random(900, 2200);
    }
  }

  // ---- hedef sekiller ----
  float tHT = 15, tHB = 13, tTilt = 0, tPr = 6, tBL = 0, tBT = 0, tBA = 3, tBR = 0;
  if (m == M_NORMAL) {
    if (d == D_DINLE) { tHT = 19; tHB = 15; tPr = 7.5f; tBL = -4; tBA = 4; }
    else if (d == D_DUSUN) { tHT = 13; tHB = 12; tPr = 5; tBR = -5; tBT = 2; }
    else if (d == D_KONUS) { tBL = -1; }
  } else if (m == M_MUTLU) {
    tHT = 14; tHB = -8; tPr = 7; tBL = -3; tBA = 4;
  } else if (m == M_UZGUN) {
    tHT = 13; tHB = 12; tTilt = -7; tPr = 7.5f; tBL = 2; tBT = -8; tBA = 1; gxT = 0; gyT = 5;
  } else if (m == M_KIZGIN) {
    tHT = 15; tHB = 9; tTilt = 9; tPr = 4.5f; tBL = 6; tBT = 9; tBA = 0; gxT = 0; gyT = 0;
  } else if (m == M_SASKIN) {
    tHT = 23; tHB = 19; tPr = 3.5f; tBL = -8; tBA = 5; gxT = 0; gyT = -1;
  } else if (m == M_UYKULU) {
    tHT = 10; tHB = 11; tTilt = -2; tPr = 6; tBL = 3; tBA = 2; gyT = 2;
  }
  if (uyaniyor) {                // uyanma: gozler iri iri acilir
    tHT = 20; tHB = 16; tTilt = 0; tPr = 8; tBL = -5; tBA = 4; gxT = 0; gyT = -1;
  }
  if (uy) {                      // uyku: kapali, asagi kavisli gozler
    tHT = -4; tHB = 6; tTilt = 0; tPr = 6; tBL = 12; tBT = 0; tBR = 0; tBA = 1; gxT = 0; gyT = 0;
  }
  if (d == D_KONUS) {            // konusma sesine tepki
    tPr += lv * 2.0f;
    tBL -= lv * 3.0f;
    tHT += lv * 2.0f;
  }

  // ---- yumusat ----
  float a = 1.0f - expf(-dt * (uy ? 2.5f : 10.0f));      // uykuya yavasca dalar
  hT += (tHT - hT) * a; hB += (tHB - hB) * a; tilt += (tTilt - tilt) * a;
  a = 1.0f - expf(-dt * 14.0f);
  pr += (tPr - pr) * a;
  a = 1.0f - expf(-dt * (uy ? 3.0f : 8.0f));
  bL += (tBL - bL) * a; bT += (tBT - bT) * a; bA += (tBA - bA) * a; bR += (tBR - bR) * a;
  a = 1.0f - expf(-dt * 20.0f);
  gx += (gxT - gx) * a; gy += (gyT - gy) * a;

  // ---- mikro hareketler ----
  if (t - sonJit > 130) {
    sonJit = t;
    jx = random(-5, 6) / 10.0f;
    jy = random(-4, 5) / 10.0f;
  }
  int oy = uy ? (int)roundf(sinf(ts * 0.9f) * 2.0f) : (int)roundf(sinf(ts * 1.6f) * 1.0f);   // nefes

  // ---- goz kirpma ----
  if (blinkBas == 0 && t >= sonrakiBlink && !uy) {
    blinkBas = t;
    blinkSure = (m == M_UYKULU) ? 520.0f : 230.0f;
    blinkSayac = (random(100) < 15) ? 2 : 1;
    sonrakiBlink = t + ((m == M_UYKULU) ? random(1800, 4000) : random(2200, 6000));
  }
  float c = 0;
  if (blinkBas != 0) {
    float x = (t - blinkBas) / blinkSure;
    if (x >= 1.0f) {
      blinkSayac--;
      if (blinkSayac > 0) blinkBas = t; else blinkBas = 0;
    } else {
      c = (x < 0.4f) ? (x / 0.4f) : ((1.0f - x) / 0.6f);
      c = c * c * (3.0f - 2.0f * c);
    }
  }

  // bakis yonune gore ust kapak takip eder
  float hTe = hT - gy * 0.5f;
  float hBe = hB + gy * 0.2f;
  float prE = pr + 0.3f * sinf(ts * 3.1f);
  int irisOn = (hB > 2.0f && hT > 3.0f) ? 1 : 0;
  int lash = irisOn;

  display.clearDisplay();
  gozCiz(0, 34, EYE_CY + oy, gx + jx, gy + jy, hTe, hBe, tilt, prE, c, irisOn, bL, bT, bA, lash);
  gozCiz(1, 94, EYE_CY + oy, gx + jx, gy + jy, hTe, hBe, tilt, prE, c, irisOn, bL + bR, bT, bA, lash);

  // uyurken yukari dogru suzulen "z z z"
  if (uy && hT < 2.0f) {
    for (int n = 0; n < 3; n++) {
      float ph = fmodf(ts * 0.45f + n / 3.0f, 1.0f);
      int sz = 2 + (int)(ph * 3.0f);
      int zx = 100 + (int)(ph * 18.0f);
      int zy = 18 - (int)(ph * 18.0f);
      display.drawLine(zx, zy, zx + sz, zy, SSD1306_WHITE);
      display.drawLine(zx + sz, zy, zx, zy + sz, SSD1306_WHITE);
      display.drawLine(zx, zy + sz, zx + sz, zy + sz, SSD1306_WHITE);
    }
  }
}

void gozGorevi(void* p) {
  Wire.begin(8, 9);
  Wire.setClock(800000);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    vTaskDelete(NULL);
  }
  irisUret();
  while (true) {
    gozAdim();
    display.display();
    vTaskDelay(6 / portTICK_PERIOD_MS);
  }
}
// ===== GOZ BITIS =====

// ---------- SES / AG ----------

void wavBaslikYaz() {
  uint32_t dataLen = gN * 2;
  uint32_t fileLen = 36 + dataLen;
  uint32_t fmtLen = 16, sr = SR, br = SR * 2;
  uint16_t fmt = 1, ch = 1, ba = 2, bits = 16;
  memcpy(wav, "RIFF", 4);
  memcpy(wav + 4, &fileLen, 4);
  memcpy(wav + 8, "WAVEfmt ", 8);
  memcpy(wav + 16, &fmtLen, 4);
  memcpy(wav + 20, &fmt, 2);
  memcpy(wav + 22, &ch, 2);
  memcpy(wav + 24, &sr, 4);
  memcpy(wav + 28, &br, 4);
  memcpy(wav + 32, &ba, 2);
  memcpy(wav + 34, &bits, 2);
  memcpy(wav + 36, "data", 4);
  memcpy(wav + 40, &dataLen, 4);
}

// ===== AYAR VE GUNCELLEME BASLA =====
void ayarYukle() {
  tercih.begin("finduk", false);
  WIFI_AD = tercih.getString("wifi", "");
  WIFI_SIFRE = tercih.getString("sifre", "");
  GEMINI_KEY = tercih.getString("gemini", "");
  CLAUDE_KEY = tercih.getString("claude", "");
  GH_TOKEN = tercih.getString("token", "");
  tercih.end();
}

bool ayarTam() {
  return WIFI_AD.length() > 0 && WIFI_SIFRE.length() > 0 && GEMINI_KEY.length() > 0 && CLAUDE_KEY.length() > 0;
}

// "wifi=ADI" gibi bir satiri isler. Basarili kayitta true doner.
bool ayarKomutIsle(String s) {
  s.trim();
  if (s.length() == 0) return false;
  if (s == "goster") {
    Serial.println(String("wifi: ") + (WIFI_AD.length() ? WIFI_AD : String("(yok)")));
    Serial.println(String("sifre: ") + (WIFI_SIFRE.length() ? "kayitli" : "(yok)"));
    Serial.println(String("gemini: ") + (GEMINI_KEY.length() ? "kayitli" : "(yok)"));
    Serial.println(String("claude: ") + (CLAUDE_KEY.length() ? "kayitli" : "(yok)"));
    Serial.println(String("token: ") + (GH_TOKEN.length() ? "kayitli" : "(yok, istege bagli)"));
    Serial.println(String("surum: ") + FW_SURUM);
    return false;
  }
  int k = s.indexOf('=');
  if (k < 0) {
    Serial.println("Anlamadim. Ornek: wifi=EvAgi");
    return false;
  }
  String ad = s.substring(0, k);
  String deger = s.substring(k + 1);
  ad.trim();
  deger.trim();
  const char* anahtar = nullptr;
  if (ad == "wifi") anahtar = "wifi";
  else if (ad == "sifre") anahtar = "sifre";
  else if (ad == "gemini") anahtar = "gemini";
  else if (ad == "claude") anahtar = "claude";
  else if (ad == "token") anahtar = "token";
  if (anahtar == nullptr) {
    Serial.println("Bilinmeyen ad. Kullanilabilir: wifi sifre gemini claude token");
    return false;
  }
  tercih.begin("finduk", false);
  tercih.putString(anahtar, deger);
  tercih.end();
  ayarYukle();
  Serial.println(String("Kaydedildi: ") + ad);
  return true;
}

// Acilista kisa sure seri ekrandan ayar kabul eder. Bilgiler eksikse tamamlanana kadar bekler.
void ayarPenceresi() {
  Serial.println();
  Serial.println("=== AYAR === Komutlar (yazip Enter'a bas):");
  Serial.println("wifi=AGADI   sifre=SIFRE   gemini=ANAHTAR   claude=ANAHTAR   token=GITHUBTOKEN (istege bagli)   goster");
  unsigned long bas = millis();
  unsigned long sonUyari = millis();
  unsigned long sure = ayarTam() ? 4000UL : 6000UL;
  while (millis() - bas < sure || !ayarTam()) {
    if (Serial.available()) {
      String satir = Serial.readStringUntil('\n');
      if (ayarKomutIsle(satir)) bas = millis();
    }
    if (!ayarTam() && millis() - sonUyari > 5000) {
      sonUyari = millis();
      Serial.println("Eksik bilgi var. wifi, sifre, gemini ve claude komutlarini yaz (goster ile bak).");
    }
    delay(10);
  }
  Serial.println("Ayar tamam, baslatiyorum...");
}

// GitHub'da yeni surum varsa indirir ve kendini yeniler
void guncellemeKontrol() {
  sonGuncelKontrol = millis();
  if (WiFi.status() != WL_CONNECTED) return;
  String taban = String(GUNCEL_URL);

  int yeni = 0;
  {
    WiFiClientSecure c;
    c.setInsecure();
    HTTPClient http;
    http.setTimeout(15000);
    if (!http.begin(c, taban + "version.txt")) return;
    if (GH_TOKEN.length() > 0) http.addHeader("Authorization", String("token ") + GH_TOKEN);
    int kod = http.GET();
    if (kod != 200) {
      Serial.println(String("(Guncelleme kontrolu: HTTP ") + kod + ")");
      http.end();
      return;
    }
    yeni = http.getString().toInt();
    http.end();
  }
  Serial.println(String("Yazilim surumu: bende ") + FW_SURUM + ", GitHub'da " + yeni);
  if (yeni <= FW_SURUM) return;

  Serial.println("Yeni surum var, indiriyorum...");
  gDurum = D_DUSUN;
  WiFiClientSecure c2;
  c2.setInsecure();
  HTTPClient h2;
  h2.setTimeout(30000);
  if (!h2.begin(c2, taban + "firmware.bin")) return;
  if (GH_TOKEN.length() > 0) h2.addHeader("Authorization", String("token ") + GH_TOKEN);
  int kod2 = h2.GET();
  if (kod2 != 200) {
    Serial.println(String("Indirme hatasi: HTTP ") + kod2);
    h2.end();
    gDurum = D_BOSTA;
    return;
  }
  int boy = h2.getSize();
  if (boy < 100000) {
    Serial.println("Dosya boyutu beklenmedik, vazgectim.");
    h2.end();
    gDurum = D_BOSTA;
    return;
  }
  if (!Update.begin(boy)) {
    Serial.println(String("Update.begin hata: ") + Update.errorString());
    h2.end();
    gDurum = D_BOSTA;
    return;
  }
  size_t yazilan = Update.writeStream(*h2.getStreamPtr());
  bool tamam = (yazilan == (size_t)boy) && Update.end() && Update.isFinished();
  h2.end();
  if (tamam) {
    Serial.println("Guncelleme tamam, yeniden basliyorum.");
    delay(800);
    ESP.restart();
  } else {
    Serial.println(String("Guncelleme basarisiz: ") + Update.errorString());
    Update.abort();
    gDurum = D_BOSTA;
  }
}
// ===== AYAR VE GUNCELLEME BITIS =====

void wifiBaglan() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);

  uint8_t mac[6];
  esp_fill_random(mac, 6);
  mac[0] = (mac[0] & 0xFE) | 0x02;
  esp_wifi_set_mac(WIFI_IF_STA, mac);

  WiFi.setSleep(false);
  Serial.println("Wi-Fi baglaniyor (yeni kimlikle)...");
  unsigned long bas = millis();
  WiFi.begin(WIFI_AD.c_str(), WIFI_SIFRE.c_str());
  while (WiFi.status() != WL_CONNECTED && millis() - bas < 120000UL) {
    delay(500);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Wi-Fi tamam. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Wi-Fi BAGLANAMADI.");
  }
}

void miknrofonKur() {
  i2s_config_t cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
  cfg.sample_rate = SR;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  cfg.dma_buf_count = 8;
  cfg.dma_buf_len = 256;
  i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);

  i2s_pin_config_t pins;
  memset(&pins, 0, sizeof(pins));
  pins.mck_io_num = I2S_PIN_NO_CHANGE;
  pins.bck_io_num = MIC_SCK;
  pins.ws_io_num = MIC_WS;
  pins.data_out_num = I2S_PIN_NO_CHANGE;
  pins.data_in_num = MIC_SD;
  i2s_set_pin(I2S_NUM_0, &pins);
}

void hoparlorKur() {
  pinMode(AMP_SD, OUTPUT);
  digitalWrite(AMP_SD, LOW);

  i2s_config_t cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  cfg.sample_rate = 24000;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  cfg.dma_buf_count = 16;
  cfg.dma_buf_len = 512;
  cfg.tx_desc_auto_clear = true;
  i2s_driver_install(I2S_NUM_1, &cfg, 0, NULL);

  i2s_pin_config_t pins;
  memset(&pins, 0, sizeof(pins));
  pins.mck_io_num = I2S_PIN_NO_CHANGE;
  pins.bck_io_num = AMP_BCLK;
  pins.ws_io_num = AMP_LRC;
  pins.data_out_num = AMP_DIN;
  pins.data_in_num = I2S_PIN_NO_CHANGE;
  i2s_set_pin(I2S_NUM_1, &pins);
}

// Ses gelene kadar bekler. zamanAsimi=0 ise sonsuza dek bekler. Ses gelirse true, sure dolarsa false.
// Sesin ilk anini kacirmamak icin son 768 ornek (48 ms) surekli saklanir
#define ONKAYIT 768
static int16_t onTampon[ONKAYIT];
static int onN = 0;

bool bekleSes(unsigned long zamanAsimi, int esik) {
  int32_t buf[256];
  size_t got = 0;
  static int16_t halka[ONKAYIT];
  memset(halka, 0, sizeof(halka));
  for (int i = 0; i < 20; i++) i2s_read(I2S_NUM_0, buf, sizeof(buf), &got, portMAX_DELAY);
  unsigned long bas = millis();
  while (true) {
    if (zamanAsimi > 0 && millis() - bas > zamanAsimi) return false;
    i2s_read(I2S_NUM_0, buf, sizeof(buf), &got, portMAX_DELAY);
    int n = got / 4;
    if (n <= 0) continue;
    if (n > 256) n = 256;
    long long top = 0;
    for (int i = 0; i < n; i++) top += (buf[i] >> 14);
    int ort = (int)(top / n);
    int tepe = 0;
    for (int i = 0; i < n; i++) {
      int v = (buf[i] >> 14) - ort;
      if (abs(v) > tepe) tepe = abs(v);
    }
    // halkayi kaydir, yeni parcayi sona ekle
    memmove(halka, halka + n, (ONKAYIT - n) * sizeof(int16_t));
    for (int i = 0; i < n; i++) {
      int32_t v = buf[i] >> 14;
      if (v > 32767) v = 32767;
      if (v < -32768) v = -32768;
      halka[ONKAYIT - n + i] = (int16_t)v;
    }
    if (tepe > esik) {
      memcpy(onTampon, halka, sizeof(halka));
      onN = ONKAYIT;
      return true;
    }
  }
}

// Konusma baslayinca kaydeder, SESSIZ_BITIS kadar sessizlik olunca (en fazla MAX_SANIYE) durur.
int kaydet() {
  int32_t buf[256];
  size_t got = 0;
  int idx = 0;
  for (int i = 0; i < onN && idx < N_MAX; i++) pcm[idx++] = onTampon[i];
  onN = 0;
  gSesliMs = 48;                         // tetikleyen parca sesliydi
  unsigned long sessizBas = 0;           // 0: su an ses var
  while (idx < N_MAX) {
    i2s_read(I2S_NUM_0, buf, sizeof(buf), &got, portMAX_DELAY);
    int n = got / 4;
    if (n <= 0) continue;
    if (n > 256) n = 256;
    long long top = 0;
    for (int i = 0; i < n; i++) top += (buf[i] >> 14);
    int ort = (int)(top / n);
    int tepe = 0;
    for (int i = 0; i < n; i++) {
      int32_t v = buf[i] >> 14;
      if (v > 32767) v = 32767;
      if (v < -32768) v = -32768;
      if (idx < N_MAX) pcm[idx++] = (int16_t)v;
      int d = abs((int)(buf[i] >> 14) - ort);
      if (d > tepe) tepe = d;
    }
    if (tepe > ESIK_BITIS) {
      sessizBas = 0;
      gSesliMs += (unsigned long)n * 1000UL / SR;
    } else if (sessizBas == 0) {
      sessizBas = millis();
    } else if (millis() - sessizBas >= SESSIZ_BITIS) {
      break;
    }
  }
  gN = idx;
  gWavBytes = 44 + gN * 2;

  long long toplam = 0;
  for (int i = 0; i < gN; i++) toplam += pcm[i];
  int ort = (gN > 0) ? (int)(toplam / gN) : 0;
  int tepe = 0;
  for (int i = 0; i < gN; i++) {
    int v = pcm[i] - ort;
    if (v > 32767) v = 32767;
    if (v < -32768) v = -32768;
    pcm[i] = (int16_t)v;
    if (abs(v) > tepe) tepe = abs(v);
  }
  wavBaslikYaz();
  return tepe;
}

size_t b64Uzunluk(size_t n) { return 4 * ((n + 2) / 3); }

int b64Kodla(const uint8_t* in, int len, char* out) {
  int o = 0;
  for (int i = 0; i < len; i += 3) {
    uint32_t v = (uint32_t)in[i] << 16;
    if (i + 1 < len) v |= (uint32_t)in[i + 1] << 8;
    if (i + 2 < len) v |= in[i + 2];
    out[o++] = B64[(v >> 18) & 63];
    out[o++] = B64[(v >> 12) & 63];
    out[o++] = (i + 1 < len) ? B64[(v >> 6) & 63] : '=';
    out[o++] = (i + 2 < len) ? B64[v & 63] : '=';
  }
  return o;
}

int b64Deger(int c) {
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= 'a' && c <= 'z') return c - 'a' + 26;
  if (c >= '0' && c <= '9') return c - '0' + 52;
  if (c == '+') return 62;
  if (c == '/') return 63;
  return -1;
}

bool yaz(WiFiClientSecure& c, const uint8_t* d, size_t n) {
  size_t gonderilen = 0;
  unsigned long t0 = millis();
  while (gonderilen < n && millis() - t0 < 20000) {
    size_t w = c.write(d + gonderilen, n - gonderilen);
    if (w > 0) {
      gonderilen += w;
      t0 = millis();
    } else {
      delay(5);
    }
  }
  return gonderilen == n;
}

String cevapOku(WiFiClientSecure& client) {
  String cevap;
  unsigned long t0 = millis();
  while ((client.connected() || client.available()) && millis() - t0 < 40000) {
    while (client.available()) cevap += (char)client.read();
    delay(10);
  }
  return cevap;
}

String geminiSesiYaziyaCevir() {
  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(30);
  if (!client.connect("generativelanguage.googleapis.com", 443)) return "BAGLANTI HATASI";

  String onek = "{\"contents\":[{\"parts\":[{\"text\":\"Bu ses kaydini Turkce olarak aynen yaziya cevir. Konusmada Finduk adinda bir robota seslenilebilir, bu ismi Findik diye degil aynen Finduk diye yaz. Sadece soylenen cumleyi yaz, baska hicbir sey ekleme. Ses yoksa BOS yaz.\"},{\"inline_data\":{\"mime_type\":\"audio/wav\",\"data\":\"";
  String sonek = "\"}}]}]}";
  size_t govde = onek.length() + b64Uzunluk(gWavBytes) + sonek.length();

  String baslik = String("POST /v1beta/models/") + MODEL + ":generateContent HTTP/1.1\r\n" +
                  "Host: generativelanguage.googleapis.com\r\n" +
                  "x-goog-api-key: " + GEMINI_KEY + "\r\n" +
                  "Content-Type: application/json\r\n" +
                  "Content-Length: " + govde + "\r\n" +
                  "Connection: close\r\n\r\n";

  if (!yaz(client, (const uint8_t*)baslik.c_str(), baslik.length())) return "YAZMA HATASI (baslik)";
  if (!yaz(client, (const uint8_t*)onek.c_str(), onek.length())) return "YAZMA HATASI (on ek)";

  static char cikis[1100];
  for (int i = 0; i < gWavBytes; i += 768) {
    int len = gWavBytes - i;
    if (len > 768) len = 768;
    int o = b64Kodla(wav + i, len, cikis);
    if (!yaz(client, (const uint8_t*)cikis, o)) return "YAZMA HATASI (ses)";
  }
  if (!yaz(client, (const uint8_t*)sonek.c_str(), sonek.length())) return "YAZMA HATASI (son ek)";

  return cevapOku(client);
}

String jsonKacis(const String& s) {
  String o;
  for (unsigned int i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '"') o += "\\\"";
    else if (c == '\\') o += "\\\\";
    else if ((unsigned char)c < 32) o += ' ';
    else o += c;
  }
  return o;
}

void hafizayaEkle(const String& soru, const String& cevap) {
  if (hN == HAFIZA) {
    for (int i = 1; i < HAFIZA; i++) {
      hSoru[i - 1] = hSoru[i];
      hCevap[i - 1] = hCevap[i];
    }
    hN = HAFIZA - 1;
  }
  hSoru[hN] = soru;
  hCevap[hN] = cevap;
  hN++;
}

String claudeCevapla(const String& soru, const String& bilgi) {
  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(30);
  if (!client.connect("api.anthropic.com", 443)) return "";

  // Gecmis konusmalar + yeni soru
  String mesajlar = "[";
  for (int i = 0; i < hN; i++) {
    mesajlar += String("{\"role\":\"user\",\"content\":\"") + jsonKacis(hSoru[i]) + "\"},";
    mesajlar += String("{\"role\":\"assistant\",\"content\":\"") + jsonKacis(hCevap[i]) + "\"},";
  }
  mesajlar += String("{\"role\":\"user\",\"content\":\"") + jsonKacis(soru) + "\"}]";

  String sistem = String(KISILIK);
  if (bilgi.length() > 0) {
    sistem += " ASAGIDA INTERNETTEN ALINMIS GUNCEL BILGI VAR. Cevabini buna dayandir. Haber ise en fazla 3 basligi kisa kisa soyle, hava ise sicakligi ve durumu soyle. Bilgi: " + bilgi;
  }
  String govde = String("{\"model\":\"") + CLAUDE_MODEL + "\",\"max_tokens\":300,\"system\":\"" +
                 jsonKacis(sistem) + "\",\"messages\":" + mesajlar + "}";

  String baslik = String("POST /v1/messages HTTP/1.1\r\n") +
                  "Host: api.anthropic.com\r\n" +
                  "x-api-key: " + CLAUDE_KEY + "\r\n" +
                  "anthropic-version: 2026-10-21\r\n" +
                  "Content-Type: application/json\r\n" +
                  "Content-Length: " + govde.length() + "\r\n" +
                  "Connection: close\r\n\r\n";

  if (!yaz(client, (const uint8_t*)baslik.c_str(), baslik.length())) return "";
  if (!yaz(client, (const uint8_t*)govde.c_str(), govde.length())) return "";
  return cevapOku(client);
}

String degerBul(const String& r, const char* anahtar) {
  String ara = String("\"") + anahtar + "\"";
  int k = r.indexOf(ara);
  while (k >= 0) {
    int j = k + ara.length();
    while (j < (int)r.length() && (r[j] == ' ' || r[j] == '\n' || r[j] == '\r')) j++;
    if (j < (int)r.length() && r[j] == ':') {
      j++;
      while (j < (int)r.length() && (r[j] == ' ' || r[j] == '\n' || r[j] == '\r')) j++;
      if (j < (int)r.length() && r[j] == '"') {
        j++;
        String s;
        while (j < (int)r.length() && r[j] != '"') {
          if (r[j] == '\\' && j + 1 < (int)r.length()) {
            j++;
            if (r[j] == 'n') s += ' ';
            else s += r[j];
          } else {
            s += r[j];
          }
          j++;
        }
        s.trim();
        return s;
      }
    }
    k = r.indexOf(ara, k + ara.length());
  }
  return "";
}

// Cevabin basindaki [duygu] etiketini ayiklar, etiketi metinden siler
int moodCoz(String& c) {
  c.trim();
  if (!c.startsWith("[")) return M_NORMAL;
  int k = c.indexOf(']');
  if (k < 0 || k > 14) return M_NORMAL;
  String e = c.substring(1, k);
  e.toLowerCase();
  c = c.substring(k + 1);
  c.trim();
  if (e.indexOf("mut") >= 0) return M_MUTLU;
  if (e.indexOf("zgun") >= 0 || e.indexOf("zgün") >= 0) return M_UZGUN;
  if (e.indexOf("gin") >= 0 || e.indexOf("gın") >= 0) return M_KIZGIN;
  if (e.indexOf("sk") >= 0 || e.indexOf("şk") >= 0) return M_SASKIN;
  if (e.indexOf("uyk") >= 0) return M_UYKULU;
  return M_NORMAL;
}

// ===== INTERNET BILGI BASLA =====
// HTTP/1.0 ile ceker (parcali aktarim olmaz), govdeyi dondurur
String httpsGetGovde(const char* host, const char* yol, int maxBayt) {
  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(15);
  if (!client.connect(host, 443)) return "";
  client.print(String("GET ") + yol + " HTTP/1.0\r\nHost: " + host +
               "\r\nUser-Agent: Finduk\r\nAccept-Encoding: identity\r\nConnection: close\r\n\r\n");
  String s;
  s.reserve(maxBayt + 64);
  unsigned long t0 = millis();
  while ((client.connected() || client.available()) && millis() - t0 < 15000 && (int)s.length() < maxBayt) {
    while (client.available() && (int)s.length() < maxBayt) s += (char)client.read();
    delay(2);
  }
  client.stop();
  int k = s.indexOf("\r\n\r\n");
  if (k >= 0) s = s.substring(k + 4);
  return s;
}

// "anahtar": 12.3 gibi sayi degerini bulur (metin degerlerini atlar)
String sayiBul(const String& r, const char* anahtar) {
  String ara = String("\"") + anahtar + "\":";
  int k = r.indexOf(ara);
  while (k >= 0) {
    int j = k + ara.length();
    while (j < (int)r.length() && r[j] == ' ') j++;
    if (j < (int)r.length() && (r[j] == '-' || (r[j] >= '0' && r[j] <= '9'))) {
      String s;
      while (j < (int)r.length() && (r[j] == '-' || r[j] == '.' || (r[j] >= '0' && r[j] <= '9'))) { s += r[j]; j++; }
      return s;
    }
    k = r.indexOf(ara, k + ara.length());
  }
  return "";
}

const char* havaYazi(int kod) {
  if (kod == 0) return "acik";
  if (kod == 1) return "az bulutlu";
  if (kod == 2) return "parcali bulutlu";
  if (kod == 3) return "kapali";
  if (kod == 45 || kod == 48) return "sisli";
  if (kod >= 51 && kod <= 57) return "cisenti yagisli";
  if (kod >= 61 && kod <= 67) return "yagmurlu";
  if (kod >= 71 && kod <= 77) return "karli";
  if (kod >= 80 && kod <= 82) return "saganak yagisli";
  if (kod == 85 || kod == 86) return "kar saganakli";
  if (kod >= 95) return "gok gurultulu firtinali";
  return "degisken";
}

// Istanbul'un anlik havasi (Open-Meteo, ucretsiz, anahtar gerekmez)
String havaBilgisi() {
  String r = httpsGetGovde("api.open-meteo.com",
      "/v1/forecast?latitude=41.01&longitude=28.97&current=temperature_2m,weather_code,wind_speed_10m&timezone=auto", 3000);
  String t = sayiBul(r, "temperature_2m");
  if (t.length() == 0) return "";
  String c = sayiBul(r, "weather_code");
  String w = sayiBul(r, "wind_speed_10m");
  String o = String("Istanbul'da su an hava ") + (int)roundf(t.toFloat()) + " derece, " + havaYazi(c.toInt());
  if (w.length() > 0) o += String(", ruzgar saatte ") + (int)roundf(w.toFloat()) + " kilometre";
  o += ".";
  return o;
}

// TRT Haber son dakika baslik(lar)i
String haberBilgisi() {
  String r = httpsGetGovde("www.trthaber.com", "/sondakika.rss", 14000);
  if (r.length() == 0) return "";
  String o;
  int n = 0;
  int k = r.indexOf("<item>");
  while (k >= 0 && n < 4) {
    int a = r.indexOf("<title>", k);
    if (a < 0) break;
    int b = r.indexOf("</title>", a);
    if (b < 0) break;
    String t = r.substring(a + 7, b);
    t.replace("<![CDATA[", "");
    t.replace("]]>", "");
    t.replace("&amp;", "&");
    t.replace("&quot;", "'");
    t.replace("&#39;", "'");
    t.trim();
    if (t.length() > 160) t = t.substring(0, 160);
    if (t.length() > 0) { n++; o += String(n) + ") " + t + " "; }
    k = b;
  }
  return o;
}

bool havaMi(const String& sd) {
  const char* k[] = {"hava durum", "hava nasil", "kac derece", "sicaklik", "yagmur", "yagacak", "kar yag", "soguk mu", "sicak mi", "hava kac"};
  for (int i = 0; i < 10; i++) if (sd.indexOf(k[i]) >= 0) return true;
  return false;
}

bool haberMi(const String& sd) {
  const char* k[] = {"haber", "son dakika", "gundem", "neler oluyor", "ne oluyor", "ne olmus"};
  for (int i = 0; i < 6; i++) if (sd.indexOf(k[i]) >= 0) return true;
  return false;
}
// ===== INTERNET BILGI BITIS =====

// ===== ISIM BASLA =====
const unsigned long SOHBET_SURE = 30000;   // adi soyledikten sonra bu sure adsiz devam edilebilir (ms)
unsigned long sohbetBitis = 0;

// Kucuk harfe cevirir, Turkce harfleri ASCII'ye indirger (i, g, s, c, o, u)
String sadelestir(const String& s) {
  String o;
  unsigned int n = s.length();
  for (unsigned int i = 0; i < n; i++) {
    unsigned char c = (unsigned char)s[i];
    if (c < 128) {
      if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
      o += (char)c;
    } else if (i + 1 < n) {
      unsigned char d = (unsigned char)s[i + 1];
      if (c == 0xC4) {
        if (d == 0xB1 || d == 0xB0) o += 'i';
        else if (d == 0x9F || d == 0x9E) o += 'g';
      } else if (c == 0xC5) {
        if (d == 0x9F || d == 0x9E) o += 's';
      } else if (c == 0xC3) {
        if (d == 0xA7 || d == 0x87) o += 'c';
        else if (d == 0xB6 || d == 0x96) o += 'o';
        else if (d == 0xBC || d == 0x9C) o += 'u';
      }
      i++;
    }
  }
  return o;
}

// Metinde "Finduk" (veya Findik gibi duyulan halleri) geciyor mu?
bool adiGeciyor(const String& metin) {
  String s = sadelestir(metin);
  const char* kaliplar[] = {"finduk", "findik", "fenduk", "fendik", "findu", "fundu", "pinduk", "binduk", "vinduk"};
  for (int i = 0; i < 9; i++) {
    if (s.indexOf(kaliplar[i]) >= 0) return true;
  }
  return false;
}
// ===== ISIM BITIS =====

// ---------- KONUSMA (TTS) ----------

int hamBayt() {
  if (rpos < rlen) return rbuf[rpos++];
  unsigned long t0 = millis();
  while (true) {
    int av = gOkuC->available();
    if (av > 0) {
      if (av > (int)sizeof(rbuf)) av = sizeof(rbuf);
      int n = gOkuC->read(rbuf, av);
      if (n > 0) {
        rlen = n;
        rpos = 1;
        return rbuf[0];
      }
    } else {
      if (!gOkuC->connected()) return -1;
      if (millis() - t0 > 30000) return -1;
      delay(1);
    }
  }
}

bool satirOku(String& s) {
  s = "";
  while (true) {
    int b = hamBayt();
    if (b < 0) return false;
    if (b == '\n') return true;
    if (b != '\r') s += (char)b;
  }
}

int govdeBayt() {
  if (gChunked && gKalan == 0) {
    String satir = "";
    while (true) {
      String s;
      if (!satirOku(s)) return -1;
      s.trim();
      if (s.length() > 0) { satir = s; break; }
    }
    gKalan = strtol(satir.c_str(), NULL, 16);
    if (gKalan == 0) return -1;
  }
  int b = hamBayt();
  if (b < 0) return -1;
  if (gChunked) gKalan--;
  return b;
}

static uint8_t sesTampon[2100];
static int sesN = 0;

void sesYaz() {
  int yaz_n = (sesN / 2) * 2;
  if (yaz_n <= 0) return;
  int16_t* s = (int16_t*)sesTampon;
  long toplamMutlak = 0;
  for (int i = 0; i < yaz_n / 2; i++) {
    s[i] = (int16_t)(((int32_t)s[i] * SES_YUZDE) / 100);
    toplamMutlak += abs((int)s[i]);
  }
  // gozler icin ses seviyesi kaydi
  float lvl = ((float)toplamMutlak / (float)(yaz_n / 2)) / 3000.0f;
  if (lvl > 1.0f) lvl = 1.0f;
  int hh = (lvH + 1) & 15;
  lvV[hh] = lvl;
  lvT[hh] = millis();
  lvH = hh;

  size_t yazildi = 0;
  i2s_write(I2S_NUM_1, sesTampon, yaz_n, &yazildi, portMAX_DELAY);
  int kalan = sesN - yaz_n;
  for (int i = 0; i < kalan; i++) sesTampon[i] = sesTampon[yaz_n + i];
  sesN = kalan;
}

void sesEkle(uint8_t b) {
  sesTampon[sesN++] = b;
  if (sesN >= 2000) sesYaz();
}

bool konus(const String& metin) {
  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(30);
  if (!client.connect("generativelanguage.googleapis.com", 443)) {
    Serial.println("Ses: baglanti hatasi");
    return false;
  }

  String govde = String("{\"contents\":[{\"parts\":[{\"text\":\"") +
                 jsonKacis(metin) +
                 "\"}]}],\"generationConfig\":{\"responseModalities\":[\"AUDIO\"],\"speechConfig\":{\"voiceConfig\":{\"prebuiltVoiceConfig\":{\"voiceName\":\"" +
                 SES_ADI + "\"}}}}}";

  String baslik = String("POST /v1beta/models/") + TTS_MODEL + ":generateContent HTTP/1.1\r\n" +
                  "Host: generativelanguage.googleapis.com\r\n" +
                  "x-goog-api-key: " + GEMINI_KEY + "\r\n" +
                  "Content-Type: application/json\r\n" +
                  "Content-Length: " + govde.length() + "\r\n" +
                  "Connection: close\r\n\r\n";

  if (!yaz(client, (const uint8_t*)baslik.c_str(), baslik.length())) return false;
  if (!yaz(client, (const uint8_t*)govde.c_str(), govde.length())) return false;

  gOkuC = &client;
  gChunked = false;
  gKalan = 0;
  rpos = 0;
  rlen = 0;

  String durum = "";
  String s;
  bool ilk = true;
  while (true) {
    if (!satirOku(s)) { Serial.println("Ses: cevap gelmedi"); return false; }
    if (ilk) { durum = s; ilk = false; }
    String kucuk = s;
    kucuk.toLowerCase();
    if (kucuk.startsWith("transfer-encoding") && kucuk.indexOf("chunked") >= 0) gChunked = true;
    if (s.length() == 0) break;
  }

  const char* kalip = "\"data\"";
  int eslesen = 0;
  int durumKod = 0;   // 0: "data" araniyor, 1: acilis tirnagi araniyor, 2: ses cozuluyor
  String bas = "";
  int dortlu[4];
  int k = 0;
  bool calindi = false;

  while (true) {
    int b = govdeBayt();
    if (b < 0) break;

    if (durumKod == 0) {
      if (bas.length() < 400) bas += (char)b;
      if (b == kalip[eslesen]) {
        eslesen++;
        if (eslesen == 6) durumKod = 1;
      } else {
        eslesen = (b == kalip[0]) ? 1 : 0;
      }
    } else if (durumKod == 1) {
      if (b == '"') {
        durumKod = 2;

        int rate = 24000;
        int ri = bas.indexOf("rate=");
        if (ri >= 0) rate = bas.substring(ri + 5).toInt();
        if (rate < 8000 || rate > 96000) rate = 24000;
        i2s_set_sample_rates(I2S_NUM_1, rate);

        gDurum = D_KONUS;
        digitalWrite(AMP_SD, HIGH);
        delay(30);
        calindi = true;
      }
    } else {
      if (b == '"') break;
      if (b == '=') continue;
      int v = b64Deger(b);
      if (v < 0) continue;
      dortlu[k++] = v;
      if (k == 4) {
        sesEkle((dortlu[0] << 2) | (dortlu[1] >> 4));
        sesEkle(((dortlu[1] & 15) << 4) | (dortlu[2] >> 2));
        sesEkle(((dortlu[2] & 3) << 6) | dortlu[3]);
        k = 0;
      }
    }
  }

  if (durumKod == 2) {
    if (k == 2) sesEkle((dortlu[0] << 2) | (dortlu[1] >> 4));
    if (k == 3) {
      sesEkle((dortlu[0] << 2) | (dortlu[1] >> 4));
      sesEkle(((dortlu[1] & 15) << 4) | (dortlu[2] >> 2));
    }
    sesYaz();
    i2s_zero_dma_buffer(I2S_NUM_1);
    delay(300);
    digitalWrite(AMP_SD, LOW);
    sesN = 0;
  }

  client.stop();
  gOkuC = nullptr;

  if (!calindi) {
    Serial.println("Ses gelmedi. Durum: " + durum);
    if (durum.indexOf("429") >= 0) Serial.println("(TTS limiti dolmus olabilir)");
    Serial.println(bas);
    return false;
  }
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  // Gozler ayri gorevde: oncelik 2, ikinci cekirdek (Wi-Fi islerinden etkilenmesin)
  xTaskCreatePinnedToCore(gozGorevi, "goz", 8192, NULL, 2, NULL, 1);

  wav = (uint8_t*)heap_caps_malloc(WAV_BYTES_MAX, MALLOC_CAP_SPIRAM);
  if (!wav) wav = (uint8_t*)malloc(WAV_BYTES_MAX);
  if (!wav) {
    Serial.println("HAFIZA YETMEDI");
    while (true) delay(1000);
  }
  pcm = (int16_t*)(wav + 44);

  ayarYukle();
  ayarPenceresi();

  miknrofonKur();
  hoparlorKur();
  wifiBaglan();
  guncellemeKontrol();
}

void loop() {
  Serial.println();
  Serial.println("Dinliyorum... (konusmaya basla)");
  gDurum = D_BOSTA;
  if (gUyku) {
    Serial.println("Uyuyorum... (bir ses cikar, uyanayim)");
    if (!bekleSes(GUNCEL_ARALIK, ESIK_UYKU)) {
      guncellemeKontrol();      // uyurken yeni surum var mi diye bak
      return;
    }
    gUyku = false;
    Serial.println("Uyandim! Simdi konusabilirsin.");
    delay(1800);                 // uyanma animasyonu
    return;
  }
  if (!bekleSes(UYKU_SURE, ESIK)) {
    if (millis() - sonGuncelKontrol > GUNCEL_ARALIK) guncellemeKontrol();
    gUyku = true;
    Serial.println("Uykuya daliyorum...");
    return;
  }
  gDurum = D_DINLE;
  Serial.println(">>> KAYIT (sustugunda biter) <<<");
  int tepe = kaydet();
  Serial.print("Kayit suresi: ");
  Serial.print(gN / (float)SR, 1);
  Serial.print(" sn, sesli: ");
  Serial.print(gSesliMs);
  Serial.print(" ms, tepe: ");
  Serial.println(tepe);
  if (gSesliMs < MIN_SESLI_MS) {
    gDurum = D_BOSTA;
    Serial.println("(Cok kisa ses, tik ya da gurultu. Yoksayiyorum.)");
    return;
  }
  gDurum = D_DUSUN;
  Serial.println("Dusunuyor...");

  String geminiHam = geminiSesiYaziyaCevir();

  if (geminiHam.indexOf("429 Too Many") >= 0) {
    gDurum = D_BOSTA;
    Serial.println("GEMINI LIMITI DOLDU (429). Sebep:");
    String m = degerBul(geminiHam, "message");
    Serial.println(m.length() ? m : geminiHam.substring(0, 300));
    Serial.println("60 saniye bekliyorum...");
    delay(60000);
    return;
  }

  String duyulan = degerBul(geminiHam, "text");
  if (duyulan.length() == 0 || duyulan == "BOS") {
    gDurum = D_BOSTA;
    Serial.println("Bir sey duyamadim. Gemini ham cevabi:");
    Serial.println(geminiHam.substring(0, 300));
    delay(3000);
    return;
  }
  Serial.print("SEN: ");
  Serial.println(duyulan);

  // Adi gecmiyorsa ve sohbet penceresi kapaliysa cevap verme
  bool adiVar = adiGeciyor(duyulan);
  if (!adiVar && millis() > sohbetBitis) {
    gDurum = D_BOSTA;
    Serial.println("(Adimi duymadim, yoksayiyorum. Hey Finduk diye basla.)");
    delay(500);
    return;
  }

  String bilgi = "";
  String sd = sadelestir(duyulan);
  bool hv = havaMi(sd);
  bool hb = (!hv && haberMi(sd));
  if (hv) {
    Serial.println("(Hava bilgisi aliniyor...)");
    bilgi = havaBilgisi();
  } else if (hb) {
    Serial.println("(Haberler aliniyor...)");
    bilgi = haberBilgisi();
  }
  if ((hv || hb) && bilgi.length() == 0) {
    bilgi = "Veri su an cekilemedi. Kullaniciya su an bakamadigini kisaca soyle.";
    Serial.println("(Bilgi alinamadi)");
  } else if (bilgi.length() > 0) {
    Serial.print("BILGI: ");
    Serial.println(bilgi);
  }

  String ham = claudeCevapla(duyulan, bilgi);
  String cevap = degerBul(ham, "text");
  if (cevap.length() > 0) {
    String hamCevap = cevap;          // etiketli hali hafiza icin
    int mood = moodCoz(cevap);        // etiketi ayiklar
    Serial.print("FINDUK (");
    Serial.print(mood);
    Serial.print("): ");
    Serial.println(cevap);
    hafizayaEkle(duyulan, hamCevap);
    gMood = mood;
    konus(cevap);
    gMoodBitis = millis() + 2500;
    sohbetBitis = millis() + SOHBET_SURE;     // 30 sn boyunca adini soylemeden devam edebilirsin
  } else {
    Serial.println("Claude cevap veremedi. Ham cevap:");
    Serial.println(ham.length() > 700 ? ham.substring(ham.length() - 700) : ham);
  }
  gDurum = D_BOSTA;
  delay(1000);
}
