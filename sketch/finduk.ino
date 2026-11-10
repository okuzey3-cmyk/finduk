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
#include <WebServer.h>
#include <ESPmDNS.h>

// Surum numarasini GitHub derlerken otomatik yazar (elle degistirme)
#ifndef FW_SURUM
#define FW_SURUM 0
#endif

// Wi-Fi bilgisi ve anahtarlar kodun icinde DEGIL, robotun hafizasinda durur (seri ekrandan bir kere yazilir)
String WIFI_AD, WIFI_SIFRE, GEMINI_KEY, CLAUDE_KEY, GH_TOKEN;
Preferences tercih;
const char* SINEMA_KONU = "finduk-sinema-fc45fb1a9b13e7aa113ad1f0";   // Sinema Odasi kontrol kanali (ntfy.sh)
const char* GUNCEL_URL = "https://raw.githubusercontent.com/okuzey3-cmyk/finduk/firmware/";
const unsigned long GUNCEL_ARALIK = 300000UL;   // 5 dk'da bir yeni surum var mi diye bakar
unsigned long sonGuncelKontrol = 0;
// Macten dinleme testi: true ise ses robotun hoparlorune degil, USB ile Mac'e gider
const bool MAC_SES = false;
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
  "Guncel bilgi (hava durumu, haberler) gerekirse sana ayrica verilir. Verilmediyse bilmedigini soyle ama asla internete bagli degilim deme. "
  "Demet'in Sinema Odasi adli film uygulamasini da kontrol edebilirsin. Kullanici istediginde cevabinin SONUNA, konusma metninden sonra, koseli parantez icinde komut yaz: [sinema:komut|ayrinti]. "
  "Komutlar: sekme|kesfet veya kadife veya yakinda veya sahne veya rafim; ara|film adi (sadece arar); ac|film adi (arar ve ilk sonucun biletini acar, ikinci sonuc icin ac|film adi|2); bilet (rastgele film bileti cek); zarf (surpriz zarf); zarfac; zarfdegis; ruh|aglamak veya gulmek veya gerilmek veya dusunmek veya fon; sec|1 (listedeki adayi sec); favori; izlerim; izledim; puanla|8|kisa not (izlendi olarak puanlar, 1-10 arasi); fragman; raf|fav veya izlenecek veya izlenen; rafcek; kadife|mesaj (uygulamanin sohbet asistanina yaz); "
  "filtre|alan=deger (alanlar: kategori film veya dizi veya turk veya yabanci, tur, sure, kimle aile veya cocuk, platform, yil recent veya 2020 veya 2010 veya 2000 veya 1990 veya 1980 veya old, puan 6 veya 7 veya 8; birden fazlasi icin ; ile ayir ornek filtre|tur=Komedi;yil=2010); filtretemizle; profil|demet veya oguzhan; yakindasec|1 (Yakinda ekranindaki n. filmi acar); ajandaac|1 (ajandadaki n. satiri acar); sahnesec|Ankara veya Izmir veya konser veya tiyatro (Sahne ekraninda sehir ya da tur secer); rafsec|1 (Rafim ekranindaki n. filmi acar); rafara|metin (rafta film adi arar, bos birakirsan aramayi temizler); rafsirala|yeni veya puan veya yil veya ad (raf siralamasi); rafmenu|biz veya atolye veya pasaport veya gunluk veya yil veya gorev veya rozet (Rafim ekranindaki bolumleri acar); geri; kapat; kaydir|asagi veya yukari; yenile; dugme|ekrandaki dugmenin yazisi. "
  "Birden fazla komutu art arda yazabilirsin. Komut kullandiysan cevapta ne yaptigini kisaca soyle. Komutu kullanici istemedikce kullanma. "
  "Saat, alarm ve not isleri de yaparsin. Su anki saat ve tarih sana verilir. Cevabinin sonuna su komutlari yazabilirsin: "
  "[zamanlayici:dakika|mesaj] ornek [zamanlayici:20|cay demlendi]; [alarm:SS:DD|mesaj] ornek [alarm:07:30|kalk]; her gun tekrar icin [alarm:07:30|kalk|her gun]; [alarmsil:hepsi] tum alarmlari siler; "
  "[not:metin] bir seyi deftere yazar; [notsil:hepsi] notlari siler. Kullanicinin kayitli notlari ve alarmlari sana verilirse onlara gore cevap ver. Komut kullandiysan ne yaptigini kisaca soyle. "
  "Namaz vakti sorulursa sana verilen vakitleri kullan, vakit icin hatirlatma istenirse alarm kur. "
  "Oyun istenirse (bilmece, kelime zinciri, sayi tahmini, yirmi soru, tabu, kim milyoner olmak ister, muzik tahmini) oyna, puani say, cevaplari kisa tut. "
  "Yazi tura veya zar istenirse rastgele sec. Istenirse fikra, saka, masal, uyku oncesi sakin hikaye anlat, bugun tarihte ne oldu soylenirse bildigin gercek bir olayi anlat. "
  "Pomodoro istenirse 25 dakikalik zamanlayici kur, bitince 5 dakikalik mola zamanlayicisi kur. Vardiya molasi hatirlatmasi icin alarm kur. Gun sonu sohbetinde bugun nasil gectigini sor. "
  "HAFIZA: Kullanici bir seyi hatirlamani isterse ya da kalici onemli bir bilgi soylerse (isim, sevdigi sey, aile, dogum gunu, yildonumu, onemli tarih) cevabinin sonuna [hafiza:kisa bilgi] yaz, tarihleri yilla yaz. Unutmani isterse [hafizasil:hepsi]. Sana verilen hafiza bilgisini dogal sekilde kullan, gereksiz yere tekrar etme. Geri sayim sorulursa hafizadaki tarih ile bugun arasini hesapla. "
  "LISTELER: [liste:alisveris|ekle|sut], [liste:yapilacak|ekle|fatura ode], [liste:alisveris|sil|sut], [liste:alisveris|temizle]. Listeler sorulursa sana verilir, oku. "
  "TARZ: Kullanici konusma tarzini degistirmek isterse [kisilik:ciddi], [kisilik:esprili], [kisilik:sakin] veya [kisilik:samimi] yaz. "
  "Dolar, euro, altin sorulursa sana verilen kurlari soyle, yorum yapma, tahmin etme. Mac, takim veya baska guncel konu sorulursa sana verilen bilgi yoksa bilmedigini soyle. Ezan vaktinde hatirlatma istenirse sana verilen vakitle alarm kur. MOLA: Kullanici belirli araliklarla mola hatirlatmasi isterse [mola:dakika] yaz (en az 15), kapatmak icin [mola:0]. PLAN: Kullanici bir gun icin plan ya da randevu soylerse [plan:Pazartesi saat 15 dis hekimi] yaz, silmek icin [plansil:dis hekimi] veya [plansil:hepsi]; bugunun gunune bak ve plani sorulunca soyle. Dini gun, kandil ya da bayram sorulursa sana verilen hicri tarih bilgisini kullan, tarih konusunda kesin konusma, Diyanet'e bakmasini hatirlat. GOZLER: Oyunda kullanici kazanirsa veya sevincli bir an olursa [goz:kutla], oyun baslarken [goz:oyun], saat gosterilmesini isterse [goz:saat] yaz. TELEFON: Kullanici bir urunun linkini telefonuna istersa [link:trendyol|erkek kot pantolon] yaz (site: trendyol, hepsiburada, amazon veya google; arama ifadesi kisa ve net, puana gore siralanir), sana belirli bir urun secme, telefonda aramayi acan link gider; kisa bir mesaj istenirse [telefon:mesaj] yaz. SUPURGE: Kullanici robot supurgeyi baslatmak, durdurmak, sarja gondermek veya bulmak isterse [supurge:baslat], [supurge:durdur], [supurge:sarj], [supurge:bul] yaz ve kisa onayla; komut yaklasik bir dakikada ulasir, hemen basladi deme, komutu gonderdim de. GERI SAYIM: Kullanici onemli bir tarih soylerse [geri:2027-02-16|evlilik yildonumu] yaz, silmek icin [gerisil:ad] veya [gerisil:hepsi]; kac gun kaldi sorulursa sana verilen sayimi soyle. Gun ozeti istenirse sana verilen saat, hava, namaz, alarm, not ve listeleri kisa ve duzenli soyle.";

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
// Ozel goz modlari: 0 yok, 1 alarm, 2 kutlama, 3 oyun, 4 saat
volatile int gOzel = 0;
volatile unsigned long gOzelBitis = 0;
volatile bool gUyku = false;   // uyku modu

Adafruit_SSD1306 display(128, 64, &Wire, -1);

const int ESIK = 3000;        // konusma algilama esigi
const int ESIK_UYKU = 5000;   // uyurken uyandirma esigi (daha yuksek ses gerekir)
const unsigned long UYKU_SURE = 60000;   // bu kadar sessizlikten sonra uyur (ms)
const int SES_YUZDE = 45;     // hoparlor sesi (varsayilan, 1-100)
volatile int gSesAyar = SES_YUZDE;   // telefondan degistirilebilir
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
  if (gOzel != 0 && t > gOzelBitis) gOzel = 0;
  int oz = uy ? 0 : gOzel;
  if (oz == 1) { tHT = 22; tHB = 18; tTilt = 0; tPr = 4; tBL = -6; tBT = 0; tBA = 5; tBR = 0; gxT = 0; gyT = -1; }
  else if (oz == 2) { tHT = 14; tHB = -8; tTilt = 0; tPr = 7; tBL = -3; tBT = 0; tBA = 4; tBR = 0; gxT = 0; gyT = -1; }
  else if (oz == 3) { tHT = 21; tHB = 17; tTilt = 0; tPr = 7; tBL = -5; tBT = 0; tBA = 4; tBR = 0; }
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

  float sx = 0;
  if (oz == 1) { oy += (int)roundf(sinf(ts * 45.0f) * 2.0f); sx = sinf(ts * 50.0f) * 3.0f; }
  if (oz == 2) oy -= (int)(fabsf(sinf(ts * 9.0f)) * 6.0f);

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

  if (oz == 4) {                 // saat modu
    long ep = (long)time(nullptr);
    display.clearDisplay();
    if (ep > 1700000000L) {
      time_t tt = (time_t)ep;
      struct tm zz;
      localtime_r(&tt, &zz);
      char hb[8];
      snprintf(hb, sizeof(hb), "%02d:%02d", zz.tm_hour, zz.tm_min);
      display.setTextColor(SSD1306_WHITE);
      display.setTextSize(4);
      display.setCursor(4, 18);
      display.print(hb);
      display.setTextSize(1);
    }
    return;
  }
  display.clearDisplay();
  gozCiz(0, 34, EYE_CY + oy, gx + jx + sx, gy + jy, hTe, hBe, tilt, prE, c, irisOn, bL, bT, bA, lash);
  gozCiz(1, 94, EYE_CY + oy, gx + jx + sx, gy + jy, hTe, hBe, tilt, prE, c, irisOn, bL + bR, bT, bA, lash);

  if (oz == 2) {                 // konfeti
    for (int n = 0; n < 14; n++) {
      int px = (n * 37 + (int)(ts * 30.0f) * (1 + n % 3)) % 128;
      int py = (n * 17 + (int)(ts * 50.0f)) % 64;
      display.fillRect(px, py, 2, 2, SSD1306_WHITE);
    }
  } else if (oz == 3) {          // parlayan yildizlar
    int ph = ((int)(ts * 4.0f)) % 4;
    const int yx[4] = {6, 120, 8, 118};
    const int yy[4] = {6, 8, 56, 55};
    for (int n = 0; n < 4; n++) {
      int r = ((n + ph) % 4 < 2) ? 3 : 1;
      display.drawFastHLine(yx[n] - r, yy[n], 2 * r + 1, SSD1306_WHITE);
      display.drawFastVLine(yx[n], yy[n] - r, 2 * r + 1, SSD1306_WHITE);
    }
  }

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
  if (!MAC_SES) digitalWrite(AMP_SD, HIGH);   // amfi hep acik, sessizken sifir verisi gider
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
    if (alarmVadesiGeldi()) return false;
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
  String zy = zamanYazi();
  if (zy.length() > 0) sistem += String(" Su anki tarih ve saat (Istanbul): ") + zy + ".";
  { String hf = satirlariAl("hafiza"); if (hf.length()) sistem += String(" Kullanici hakkinda hafizandaki bilgiler: ") + hf + ".";
    String kt = nvsOku("kisilik"); if (kt.length()) sistem += String(" Konusma tarzin: ") + kt + ".";
    if (sesYuzdeSimdi() < gSesAyar) sistem += " Gece oldu, sakin ve kisa konus."; }
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

// ===== SINEMA ODASI KONTROLU BASLA =====
// Komutu ntfy.sh kanalina yollar, Sinema Odasi uygulamasi dinleyip uygular
void sinemaGonder(const String& k) {
  WiFiClientSecure c;
  c.setInsecure();
  HTTPClient http;
  http.setTimeout(8000);
  if (!http.begin(c, String("https://ntfy.sh/") + SINEMA_KONU)) return;
  http.addHeader("Content-Type", "text/plain; charset=utf-8");
  int kod = http.POST((uint8_t*)k.c_str(), k.length());
  Serial.println(String("SINEMA komutu: ") + k + " -> HTTP " + kod);
  http.end();
}

// Uygulamanin son bildirdigi ekran durumu
String sinemaDurum() {
  String r = httpsGetGovde("ntfy.sh", (String("/") + SINEMA_KONU + "-durum/json?poll=1&since=latest").c_str(), 6000);
  return degerBul(r, "message");
}

bool sinemaMi(const String& sd) {
  const char* k[] = {"film", "bilet", "sinema", "dizi", "fragman", "zarf", "favori", "izle", "oyuncu", "puan", "raf", "kadife", "aday", "filtre", "ekran", "uygulama", "vizyon", "sahne", "ajanda", "yakinda", "pasaport", "rozet", "gunluk", "gorev"};
  for (int i = 0; i < 24; i++) if (sd.indexOf(k[i]) >= 0) return true;
  return false;
}

// Cevaptaki [sinema:..] [alarm:..] [not:..] gibi komut etiketlerini ayiklar, metinden siler. Kac komut bulundugunu dondurur.
int komutlariAyikla(String& c, String* liste, int maks) {
  int n = 0;
  int bas = 0;
  while (true) {
    int a = c.indexOf('[', bas);
    if (a < 0) break;
    int b = c.indexOf(']', a);
    if (b < 0) break;
    int k = c.indexOf(':', a);
    if (k < 0 || k > b) { bas = a + 1; continue; }
    String ad = c.substring(a + 1, k);
    if (ad == "sinema" || ad == "zamanlayici" || ad == "alarm" || ad == "alarmsil" || ad == "not" || ad == "notsil" || ad == "hafiza" || ad == "hafizasil" || ad == "liste" || ad == "kisilik" || ad == "goz" || ad == "mola" || ad == "plan" || ad == "plansil" || ad == "link" || ad == "supurge" || ad == "telefon" || ad == "geri" || ad == "gerisil") {
      String icerik = c.substring(a + 1, b);
      icerik.trim();
      if (n < maks) liste[n++] = icerik;
      c.remove(a, b - a + 1);
      bas = a;
    } else {
      bas = a + 1;
    }
  }
  c.trim();
  return n;
}
// ===== SINEMA ODASI KONTROLU BITIS =====

// ===== ZAMAN, ALARM, NOT, NAMAZ BASLA =====
const int ALARM_MAX = 5;
long aEpoch[ALARM_MAX];
bool aTekrar[ALARM_MAX];
String aMsj[ALARM_MAX];
int gSonSelamGun = -1;

long simdiEpoch() {
  long t = (long)time(nullptr);
  return (t > 1700000000L) ? t : 0;
}

String zamanYazi() {
  long t = simdiEpoch();
  if (t == 0) return "";
  time_t tt = (time_t)t;
  struct tm z;
  localtime_r(&tt, &z);
  const char* gun[] = {"Pazar", "Pazartesi", "Sali", "Carsamba", "Persembe", "Cuma", "Cumartesi"};
  const char* ay[] = {"Ocak", "Subat", "Mart", "Nisan", "Mayis", "Haziran", "Temmuz", "Agustos", "Eylul", "Ekim", "Kasim", "Aralik"};
  char b[80];
  snprintf(b, sizeof(b), "%d %s %d %s saat %02d:%02d", z.tm_mday, ay[z.tm_mon], z.tm_year + 1900, gun[z.tm_wday], z.tm_hour, z.tm_min);
  return String(b);
}

void zamanBaslat() {
  configTzTime("TRT-3", "pool.ntp.org", "time.google.com");   // Istanbul UTC+3
  for (int i = 0; i < 16 && simdiEpoch() == 0; i++) delay(500);
  Serial.println(simdiEpoch() ? String("Saat: ") + zamanYazi() : String("(Saat henuz alinamadi)"));
}

void alarmKaydet() {
  String s;
  for (int i = 0; i < ALARM_MAX; i++) {
    if (aEpoch[i] > 0) s += String(aEpoch[i]) + "|" + (aTekrar[i] ? "1" : "0") + "|" + aMsj[i] + "\n";
  }
  tercih.begin("finduk", false);
  tercih.putString("alarmlar", s);
  tercih.end();
}

void alarmYukle() {
  for (int i = 0; i < ALARM_MAX; i++) { aEpoch[i] = 0; aTekrar[i] = false; aMsj[i] = ""; }
  tercih.begin("finduk", false);
  String s = tercih.getString("alarmlar", "");
  tercih.end();
  int n = 0;
  int bas = 0;
  while (bas < (int)s.length() && n < ALARM_MAX) {
    int e = s.indexOf('\n', bas);
    if (e < 0) e = s.length();
    String satir = s.substring(bas, e);
    bas = e + 1;
    int p1 = satir.indexOf('|');
    int p2 = satir.indexOf('|', p1 + 1);
    if (p1 < 0 || p2 < 0) continue;
    aEpoch[n] = satir.substring(0, p1).toInt();
    aTekrar[n] = satir.substring(p1 + 1, p2) == "1";
    aMsj[n] = satir.substring(p2 + 1);
    n++;
  }
}

String temizMetin(String m, int maks) {
  m.replace("|", " ");
  m.replace("\n", " ");
  m.trim();
  if ((int)m.length() > maks) m = m.substring(0, maks);
  return m;
}

void alarmEkle(long epoch, bool tekrar, const String& msj) {
  int yer = -1;
  for (int i = 0; i < ALARM_MAX; i++) if (aEpoch[i] == 0) { yer = i; break; }
  if (yer < 0) {                         // dolu: en eskiyi at
    for (int i = 1; i < ALARM_MAX; i++) { aEpoch[i - 1] = aEpoch[i]; aTekrar[i - 1] = aTekrar[i]; aMsj[i - 1] = aMsj[i]; }
    yer = ALARM_MAX - 1;
  }
  aEpoch[yer] = epoch;
  aTekrar[yer] = tekrar;
  aMsj[yer] = temizMetin(msj, 100);
  alarmKaydet();
  Serial.println(String("ALARM KURULDU: ") + (epoch - simdiEpoch()) + " sn sonra, mesaj: " + aMsj[yer]);
}

void zamanlayiciKur(const String& ar) {
  int p = ar.indexOf('|');
  int dk = ar.substring(0, p < 0 ? ar.length() : p).toInt();
  String m = (p < 0) ? String("") : ar.substring(p + 1);
  long now = simdiEpoch();
  if (dk <= 0 || dk > 1440 || now == 0) { Serial.println("(Zamanlayici kurulamadi)"); return; }
  alarmEkle(now + dk * 60L, false, m);
}

void alarmKur(const String& ar) {
  int p1 = ar.indexOf('|');
  String z = (p1 < 0) ? ar : ar.substring(0, p1);
  String rest = (p1 < 0) ? String("") : ar.substring(p1 + 1);
  int c = z.indexOf(':');
  long now = simdiEpoch();
  if (c < 0 || now == 0) { Serial.println("(Alarm kurulamadi)"); return; }
  int sa = z.substring(0, c).toInt();
  int dk = z.substring(c + 1).toInt();
  if (sa < 0 || sa > 23 || dk < 0 || dk > 59) { Serial.println("(Alarm saati gecersiz)"); return; }
  bool tekrar = rest.indexOf("her gun") >= 0;
  String m = rest;
  int p2 = m.indexOf('|');
  if (p2 >= 0) m = m.substring(0, p2);
  if (m.indexOf("her gun") >= 0) m = "";
  time_t tt = (time_t)now;
  struct tm zz;
  localtime_r(&tt, &zz);
  zz.tm_hour = sa; zz.tm_min = dk; zz.tm_sec = 0;
  long hedef = (long)mktime(&zz);
  if (hedef <= now) hedef += 86400L;
  alarmEkle(hedef, tekrar, m);
}

void alarmTemizle() {
  for (int i = 0; i < ALARM_MAX; i++) { aEpoch[i] = 0; aTekrar[i] = false; aMsj[i] = ""; }
  alarmKaydet();
  Serial.println("(Alarmlar silindi)");
}

String alarmListesi() {
  String o;
  for (int i = 0; i < ALARM_MAX; i++) {
    if (aEpoch[i] <= 0) continue;
    time_t tt = (time_t)aEpoch[i];
    struct tm z;
    localtime_r(&tt, &z);
    char b[24];
    snprintf(b, sizeof(b), "%02d:%02d", z.tm_hour, z.tm_min);
    o += String(b) + (aTekrar[i] ? " (her gun)" : "") + (aMsj[i].length() ? String(" ") + aMsj[i] : String("")) + "; ";
  }
  return o;
}

bool molaVadesi();
bool alarmVadesiGeldi() {
  if (molaVadesi()) return true;
  long now = simdiEpoch();
  if (now == 0) return false;
  for (int i = 0; i < ALARM_MAX; i++) if (aEpoch[i] > 0 && now >= aEpoch[i]) return true;
  return false;
}

// Kisa sinus bip sesi (alarm icin)
volatile int gBipYuzde = 45;
void bip(int hz, int ms) {
  static int16_t b[256];
  int toplam = 24000 * ms / 1000;
  int yazilan = 0;
  float faz = 0;
  float adim = 2.0f * 3.14159265f * hz / 24000.0f;
  while (yazilan < toplam) {
    int n = toplam - yazilan;
    if (n > 256) n = 256;
    for (int i = 0; i < n; i++) {
      int k = yazilan + i;
      float zarf = 1.0f;
      if (k < 200) zarf = k / 200.0f;
      if (toplam - k < 200) zarf = (toplam - k) / 200.0f;
      b[i] = (int16_t)(sinf(faz) * 9000.0f * zarf * gBipYuzde / 100.0f);
      faz += adim;
    }
    size_t w = 0;
    i2s_write(I2S_NUM_1, b, n * 2, &w, portMAX_DELAY);
    yazilan += n;
  }
}

void sus(int ms) {
  static int16_t b[256];
  memset(b, 0, sizeof(b));
  int toplam = 24000 * ms / 1000;
  while (toplam > 0) {
    int n = toplam > 256 ? 256 : toplam;
    size_t w = 0;
    i2s_write(I2S_NUM_1, b, n * 2, &w, portMAX_DELAY);
    toplam -= n;
  }
}

// Vadesi gelen alarm varsa caldirir. Caldiysa true doner.
void molaCal();
bool alarmCal() {
  long now = simdiEpoch();
  if (now == 0) return false;
  if (molaVadesi()) { molaCal(); return true; }
  for (int i = 0; i < ALARM_MAX; i++) {
    if (aEpoch[i] <= 0 || now < aEpoch[i]) continue;
    String m = aMsj[i];
    long gec = now - aEpoch[i];
    if (aTekrar[i]) {
      while (aEpoch[i] <= now) aEpoch[i] += 86400L;
    } else {
      aEpoch[i] = 0;
    }
    alarmKaydet();
    if (gec > 600) continue;             // robot kapaliyken kacan alarm, calma
    Serial.println(String("ALARM CALIYOR: ") + m);
    gUyku = false;
    gOzelBitis = millis() + 40000;
    gOzel = 1;
    gMood = M_MUTLU;
    gMoodBitis = millis() + 5000;
    gDurum = D_KONUS;
    i2s_set_sample_rates(I2S_NUM_1, 24000);
    for (int r = 0; r < 5; r++) {                 // kademeli alarm: her turda ses biraz yukselir
      int v = SES_YUZDE * (50 + r * 25) / 100;
      gBipYuzde = v > 90 ? 90 : v;
      bip(880, 160); sus(90); bip(1175, 160); sus(140);
    }
    gBipYuzde = SES_YUZDE;
    sus(300);
    if (WiFi.status() == WL_CONNECTED) konus(m.length() ? String("Hatirlatma. ") + m : String("Zamanlayici doldu!"));
    gOzel = 0;
    gDurum = D_BOSTA;
    return true;
  }
  return false;
}

// Notlar (NVS'de, en fazla 10)
void notEkle(const String& metin) {
  String m = temizMetin(metin, 140);
  if (m.length() == 0) return;
  tercih.begin("finduk", false);
  String s = tercih.getString("notlar", "");
  s += m + "\n";
  int say = 0;
  for (unsigned int i = 0; i < s.length(); i++) if (s[i] == '\n') say++;
  while (say > 10) {
    int e = s.indexOf('\n');
    s = s.substring(e + 1);
    say--;
  }
  tercih.putString("notlar", s);
  tercih.end();
  Serial.println(String("NOT KAYDEDILDI: ") + m);
}

void notTemizle() {
  tercih.begin("finduk", false);
  tercih.putString("notlar", "");
  tercih.end();
  Serial.println("(Notlar silindi)");
}

String notlariAl() {
  tercih.begin("finduk", false);
  String s = tercih.getString("notlar", "");
  tercih.end();
  s.replace("\n", "; ");
  return s;
}

// Diyanet vakitleri (Aladhan, yontem 13), Istanbul
String namazBilgisi() {
  String r = httpsGetGovde("api.aladhan.com", "/v1/timingsByCity?city=Istanbul&country=Turkey&method=13", 6000);
  String im = degerBul(r, "Imsak");
  String gu = degerBul(r, "Sunrise");
  String og = degerBul(r, "Dhuhr");
  String ik = degerBul(r, "Asr");
  String ak = degerBul(r, "Maghrib");
  String ya = degerBul(r, "Isha");
  if (im.length() < 5 || ak.length() < 5) return "";
  return String("Bugun Istanbul icin Diyanet vakitleri: Imsak ") + im.substring(0, 5) + ", Gunes " + gu.substring(0, 5) +
         ", Ogle " + og.substring(0, 5) + ", Ikindi " + ik.substring(0, 5) + ", Aksam " + ak.substring(0, 5) + ", Yatsi " + ya.substring(0, 5) + ".";
}

bool namazMi(const String& sd) {
  const char* k[] = {"namaz", "ezan", "imsak", "iftar", "sahur", "ikindi", "yatsi", "vakit"};
  for (int i = 0; i < 8; i++) if (sd.indexOf(k[i]) >= 0) return true;
  return false;
}

bool notMi(const String& sd) {
  const char* k[] = {"not", "hatirla", "unutma", "defter"};
  for (int i = 0; i < 4; i++) if (sd.indexOf(k[i]) >= 0) return true;
  return false;
}

bool alarmMi(const String& sd) {
  const char* k[] = {"alarm", "zamanlayici", "hatirlat", "dakika sonra", "saat sonra", "uyandir", "sayac"};
  for (int i = 0; i < 7; i++) if (sd.indexOf(k[i]) >= 0) return true;
  return false;
}

// Komut etiketini isler
void komutIsle(const String& k) {
  int c = k.indexOf(':');
  if (c < 0) return;
  String ad = k.substring(0, c);
  String ar = k.substring(c + 1);
  ar.trim();
  if (ad == "sinema") sinemaGonder(ar);
  else if (ad == "zamanlayici") zamanlayiciKur(ar);
  else if (ad == "alarm") alarmKur(ar);
  else if (ad == "alarmsil") alarmTemizle();
  else if (ad == "not") notEkle(ar);
  else if (ad == "notsil") notTemizle();
  else if (ad == "hafiza") satirEkle("hafiza", ar, 30);
  else if (ad == "hafizasil") { if (ar == "hepsi") nvsYaz("hafiza", ""); else satirSil("hafiza", ar); }
  else if (ad == "liste") listeIsle(ar);
  else if (ad == "kisilik") nvsYaz("kisilik", temizMetin(ar, 20));
  else if (ad == "mola") molaKur(ar);
  else if (ad == "plan") satirEkle("plan", ar, 20);
  else if (ad == "link") linkGonder(ar);
  else if (ad == "supurge") { if (!supurgeGonder(ar)) bildir("Supurge komutu gonderilemedi (anahtar eksik olabilir)"); }
  else if (ad == "telefon") bildir(ar);
  else if (ad == "geri") geriSayimEkle(ar);
  else if (ad == "gerisil") { if (ar == "hepsi") nvsYaz("geri", ""); else satirSil("geri", ar); }
  else if (ad == "plansil") { if (ar == "hepsi") nvsYaz("plan", ""); else satirSil("plan", ar); }
  else if (ad == "goz") {
    int z = 0; unsigned long sure = 8000;
    if (ar == "kutla") { z = 2; sure = 6000; }
    else if (ar == "oyun") { z = 3; sure = 20000; }
    else if (ar == "saat") { z = 4; sure = 10000; }
    gOzelBitis = millis() + sure;
    gOzel = z;
  }
}

// Sabah ilk uyanista hava ve haber ozetli selam
void sabahSelami() {
  long now = simdiEpoch();
  if (now == 0 || WiFi.status() != WL_CONNECTED) return;
  time_t tt = (time_t)now;
  struct tm z;
  localtime_r(&tt, &z);
  if (z.tm_hour < 5 || z.tm_hour >= 11 || z.tm_yday == gSonSelamGun) return;
  gSonSelamGun = z.tm_yday;
  gDurum = D_DUSUN;
  String bilgi = String("Sabah selami ver: gunaydin de, havayi ve gunun bir iki onemli haberini cok kisa soyle. Kayitli plan varsa bugune ait olani da soyle. Plan: ") + planAl() + ". " + havaBilgisi() + " " + haberBilgisi();
  String ham = claudeCevapla("(Oguzhan az once uyandi, sabah selami ver)", bilgi);
  String cevap = degerBul(ham, "text");
  if (cevap.length() > 0) {
    String tut[4];
    komutlariAyikla(cevap, tut, 4);
    int mood = moodCoz(cevap);
    Serial.print("SABAH SELAMI: ");
    Serial.println(cevap);
    gMood = mood;
    konus(cevap);
    gMoodBitis = millis() + 2500;
  }
  gDurum = D_BOSTA;
}
// ===== ZAMAN, ALARM, NOT, NAMAZ BITIS =====

// ===== HAFIZA, LISTE, TARZ BASLA =====
String nvsOku(const char* anahtar) {
  tercih.begin("finduk", false);
  String v = tercih.getString(anahtar, "");
  tercih.end();
  return v;
}

void nvsYaz(const char* anahtar, const String& v) {
  tercih.begin("finduk", false);
  tercih.putString(anahtar, v);
  tercih.end();
}

// satir satir tutulan liste: sona ekler, en fazla maks satir
void satirEkle(const char* anahtar, const String& metin, int maks) {
  String m = temizMetin(metin, 140);
  if (m.length() == 0) return;
  String s = nvsOku(anahtar);
  if (s.indexOf(m + "\n") >= 0) return;
  s += m + "\n";
  int say = 0;
  for (unsigned int i = 0; i < s.length(); i++) if (s[i] == '\n') say++;
  while (say > maks) { s = s.substring(s.indexOf('\n') + 1); say--; }
  nvsYaz(anahtar, s);
}

void satirSil(const char* anahtar, const String& parca) {
  String s = nvsOku(anahtar);
  String yeni = "";
  int bas = 0;
  String p = parca;
  p.toLowerCase();
  while (bas < (int)s.length()) {
    int e = s.indexOf('\n', bas);
    if (e < 0) e = s.length();
    String satir = s.substring(bas, e);
    bas = e + 1;
    String k = satir;
    k.toLowerCase();
    if (p.length() > 0 && k.indexOf(p) >= 0) continue;
    if (satir.length()) yeni += satir + "\n";
  }
  nvsYaz(anahtar, yeni);
}

String satirlariAl(const char* anahtar) {
  String s = nvsOku(anahtar);
  s.replace("\n", "; ");
  return s;
}

const char* listeAnahtari(const String& ad) {
  return (ad.indexOf("alis") >= 0 || ad.indexOf("market") >= 0) ? "l_alis" : "l_yap";
}

void listeIsle(const String& ar) {
  int p1 = ar.indexOf('|');
  if (p1 < 0) return;
  String ad = ar.substring(0, p1);
  String rest = ar.substring(p1 + 1);
  int p2 = rest.indexOf('|');
  String islem = (p2 < 0) ? rest : rest.substring(0, p2);
  String metin = (p2 < 0) ? String("") : rest.substring(p2 + 1);
  islem.trim();
  const char* k = listeAnahtari(ad);
  if (islem == "ekle") satirEkle(k, metin, 25);
  else if (islem == "sil") satirSil(k, metin);
  else if (islem == "temizle") nvsYaz(k, "");
  Serial.println(String("LISTE ") + ad + " " + islem + " " + metin);
}

String listeleriAl() {
  String a = satirlariAl("l_alis");
  String y = satirlariAl("l_yap");
  return String("Alisveris listesi: ") + (a.length() ? a : String("(bos)")) + ". Yapilacaklar: " + (y.length() ? y : String("(bos)")) + ".";
}

// Gece sessiz saat: 23:00-07:00 arasi ses kisik
int sesYuzdeSimdi() {
  int v = gSesAyar;
  long t = simdiEpoch();
  if (t == 0) return v;
  time_t tt = (time_t)t;
  struct tm z;
  localtime_r(&tt, &z);
  if (z.tm_hour >= 23 || z.tm_hour < 7) return v / 2 > 8 ? v / 2 : 8;
  return v;
}

bool listeMi(const String& sd) {
  const char* k[] = {"liste", "alisveris", "yapilacak", "market", "gorev"};
  for (int i = 0; i < 5; i++) if (sd.indexOf(k[i]) >= 0) return true;
  return false;
}

bool ozetMi(const String& sd) {
  const char* k[] = {"gun ozeti", "bugun ne var", "bugun neler var", "gunun ozeti", "ozet ver", "gunaydin"};
  for (int i = 0; i < 6; i++) if (sd.indexOf(k[i]) >= 0) return true;
  return false;
}
// ===== HAFIZA, LISTE, TARZ BITIS =====

// ===== FINANS, HAFTALIK HAVA BASLA =====
String finansBilgisi() {
  String r = httpsGetGovde("open.er-api.com", "/v6/latest/USD", 6000);
  String t = sayiBul(r, "TRY");
  String e = sayiBul(r, "EUR");
  if (t.length() == 0) return "";
  float usd = t.toFloat();
  String o = String("Guncel kurlar: 1 dolar ") + String(usd, 2) + " lira";
  if (e.length() > 0 && e.toFloat() > 0) o += String(", 1 euro ") + String(usd / e.toFloat(), 2) + " lira";
  String g = httpsGetGovde("api.gold-api.com", "/price/XAU", 6000);
  String gp = sayiBul(g, "price");
  if (gp.length() > 0) o += String(", gram altin yaklasik ") + (int)roundf(gp.toFloat() / 31.1035f * usd) + " lira";
  o += ". Bunlar piyasa verisi, yatirim tavsiyesi degil.";
  return o;
}

bool finansMi(const String& sd) {
  const char* k[] = {"dolar", "euro", "kur ", "altin", "doviz", "borsa", "bitcoin"};
  for (int i = 0; i < 7; i++) if (sd.indexOf(k[i]) >= 0) return true;
  return false;
}

// "temperature_2m_max":[a,b,...] dizisinden n. degeri alir
String diziDeger(const String& r, const char* anahtar, int n) {
  String ara = String("\"") + anahtar + "\":[";
  int k = r.indexOf(ara);
  if (k < 0) return "";
  int j = k + ara.length();
  for (int i = 0; i < n; i++) {
    j = r.indexOf(',', j);
    if (j < 0) return "";
    j++;
  }
  String v;
  while (j < (int)r.length() && r[j] != ',' && r[j] != ']') { v += r[j]; j++; }
  return v;
}

String haftalikHava() {
  String r = httpsGetGovde("api.open-meteo.com",
      "/v1/forecast?latitude=41.01&longitude=28.97&daily=temperature_2m_max,temperature_2m_min,weather_code&forecast_days=5&timezone=auto", 4000);
  String o = "Istanbul onumuzdeki gunler: ";
  const char* gun[] = {"bugun", "yarin", "2 gun sonra", "3 gun sonra", "4 gun sonra"};
  bool var = false;
  for (int i = 0; i < 5; i++) {
    String mx = diziDeger(r, "temperature_2m_max", i);
    String mn = diziDeger(r, "temperature_2m_min", i);
    String wc = diziDeger(r, "weather_code", i);
    if (mx.length() == 0 || mn.length() == 0) continue;
    var = true;
    o += String(gun[i]) + " " + (int)roundf(mn.toFloat()) + " ile " + (int)roundf(mx.toFloat()) + " derece, " + havaYazi(wc.toInt()) + "; ";
  }
  return var ? o : String("");
}

bool haftalikMi(const String& sd) {
  if (!havaMi(sd) && sd.indexOf("hava") < 0) return false;
  const char* k[] = {"hafta", "yarin", "gelecek gun", "onumuzdeki"};
  for (int i = 0; i < 4; i++) if (sd.indexOf(k[i]) >= 0) return true;
  return false;
}
// ===== FINANS, HAFTALIK HAVA BITIS =====

// ===== TRAFIK, MAC BASLA =====
// "anahtar": 45 veya "anahtar":"45.2" gibi degeri sayi olarak alir
String gevsekSayi(const String& r, const char* anahtar) {
  String ara = String("\"") + anahtar + "\"";
  int k = r.indexOf(ara);
  if (k < 0) return "";
  int j = k + ara.length();
  while (j < (int)r.length() && (r[j] == ' ' || r[j] == ':' || r[j] == '"')) j++;
  String v;
  while (j < (int)r.length() && (r[j] == '-' || r[j] == '.' || (r[j] >= '0' && r[j] <= '9'))) { v += r[j]; j++; }
  return v;
}

String trafikBilgisi() {
  String r = httpsGetGovde("tkmservices.ibb.gov.tr", "/web/api/TrafficData/v1/TrafficIndex_Sc1_Cont", 3000);
  String t = gevsekSayi(r, "TI");
  String av = gevsekSayi(r, "TI_Av");
  String an = gevsekSayi(r, "TI_An");
  if (t.length() == 0) return "";
  String o = String("Istanbul trafik yogunlugu su an yuzde ") + (int)roundf(t.toFloat());
  if (av.length() > 0) o += String(", Avrupa yakasi yuzde ") + (int)roundf(av.toFloat());
  if (an.length() > 0) o += String(", Anadolu yakasi yuzde ") + (int)roundf(an.toFloat());
  o += ". (Yuzde 0-30 akici, 30-60 orta, 60 ustu yogun.)";
  return o;
}

bool trafikMi(const String& sd) {
  const char* k[] = {"trafik", "yol durumu", "yollar nasil", "kopru"};
  for (int i = 0; i < 4; i++) if (sd.indexOf(k[i]) >= 0) return true;
  return false;
}

// TheSportsDB (ucretsiz): Super Lig son ve siradaki maclar
String macListesi(const char* yol, int en, const char* baslik) {
  String r = httpsGetGovde("www.thesportsdb.com", yol, 14000);
  if (r.length() < 50) return "";
  String o = String(baslik) + ": ";
  int bas = 0, say = 0;
  while (say < en) {
    int a = r.indexOf("{\"idEvent\"", bas);
    if (a < 0) break;
    int b = r.indexOf("{\"idEvent\"", a + 10);
    String seg = (b < 0) ? r.substring(a) : r.substring(a, b);
    bas = (b < 0) ? r.length() : b;
    String ev = degerBul(seg, "strHomeTeam");
    String dep = degerBul(seg, "strAwayTeam");
    String tarih = degerBul(seg, "dateEvent");
    if (ev.length() == 0 || dep.length() == 0) continue;
    String sh = degerBul(seg, "intHomeScore");
    String sa = degerBul(seg, "intAwayScore");
    o += tarih + " " + ev;
    if (sh.length() > 0 && sa.length() > 0) o += String(" ") + sh + "-" + sa + " ";
    else o += " - ";
    o += dep + "; ";
    say++;
  }
  return say > 0 ? o : String("");
}

String macBilgisi() {
  String a = macListesi("/api/v1/json/3/eventspastleague.php?id=4339", 8, "Super Lig son maclar");
  String b = macListesi("/api/v1/json/3/eventsnextleague.php?id=4339", 6, "Super Lig siradaki maclar");
  if (a.length() == 0 && b.length() == 0) return "";
  return a + " " + b + " Bu liste eksik olabilir, emin olmadigin skoru kesin soyleme.";
}

bool macMi(const String& sd) {
  const char* k[] = {"mac ", "maci", "maclar", "skor", "super lig", "galatasaray", "fenerbahce", "besiktas", "trabzonspor", "kac kac", "puan durumu"};
  for (int i = 0; i < 11; i++) if (sd.indexOf(k[i]) >= 0) return true;
  return false;
}
// ===== TRAFIK, MAC BITIS =====

// ===== TELEFON AYAR SAYFASI, BILDIRIM BASLA =====
WebServer webSunucu(80);

String htmlKacir(String m) {
  m.replace("&", "&amp;");
  m.replace("<", "&lt;");
  m.replace(">", "&gt;");
  return m;
}

String resetNedeni() {
  esp_reset_reason_t r = esp_reset_reason();
  switch (r) {
    case ESP_RST_POWERON: return "normal acilis (fis takildi)";
    case ESP_RST_SW: return "yazilim yeniden baslatti (guncelleme)";
    case ESP_RST_PANIC: return "HATA: program coktu";
    case ESP_RST_INT_WDT: case ESP_RST_TASK_WDT: case ESP_RST_WDT: return "HATA: program takildi (watchdog)";
    case ESP_RST_BROWNOUT: return "HATA: voltaj dustu, guc kaynagi zayif";
    default: return "diger";
  }
}

// Telefona bildirim (ntfy uygulamasinda <konu>-robot kanalini takip et)
void bildir(const String& mesaj) {
  if (WiFi.status() != WL_CONNECTED) return;
  WiFiClientSecure c;
  c.setInsecure();
  HTTPClient http;
  http.setTimeout(6000);
  if (!http.begin(c, String("https://ntfy.sh/") + SINEMA_KONU + "-robot")) return;
  http.addHeader("Content-Type", "text/plain; charset=utf-8");
  http.addHeader("Title", "Finduk");
  int kod = http.POST((uint8_t*)mesaj.c_str(), mesaj.length());
  Serial.println(String("Bildirim: ") + mesaj + " -> HTTP " + kod);
  http.end();
}

void webAna() {
  String h = "<!doctype html><html><head><meta charset=utf-8><meta name=viewport content='width=device-width,initial-scale=1'>"
             "<title>Finduk</title><style>body{font-family:-apple-system,sans-serif;margin:16px;max-width:520px}"
             "h2{margin:18px 0 6px}input,select,button{font-size:18px;padding:8px;margin:4px 0}"
             "button{background:#222;color:#fff;border:0;border-radius:8px}.k{background:#eee;padding:8px;border-radius:8px;font-size:15px}"
             "</style></head><body><h1>Finduk</h1>";
  h += String("<p>Surum ") + FW_SURUM + " · " + htmlKacir(zamanYazi()) + "</p>";
  h += "<h2>Ses seviyesi</h2><form action=/ayar><input type=range name=ses min=10 max=90 value=" + String((int)gSesAyar) + " style='width:100%'><button>Kaydet</button></form>";
  h += "<h2>Konusma tarzi</h2><form action=/ayar><select name=tarz><option>samimi</option><option>esprili</option><option>ciddi</option><option>sakin</option></select><button>Kaydet</button></form>";
  h += String("<p>Su an: ") + htmlKacir(nvsOku("kisilik")) + "</p>";
  h += "<h2>Hafiza</h2><div class=k>" + htmlKacir(satirlariAl("hafiza")) + "</div><form action=/sil><input type=hidden name=ne value=hafiza><button>Hafizayi sil</button></form>";
  h += "<h2>Notlar</h2><div class=k>" + htmlKacir(notlariAl()) + "</div><form action=/sil><input type=hidden name=ne value=not><button>Notlari sil</button></form>";
  h += "<h2>Listeler</h2><div class=k>" + htmlKacir(listeleriAl()) + "</div><form action=/sil><input type=hidden name=ne value=liste><button>Listeleri sil</button></form>";
  h += String("<h2>Supurge anahtari</h2><p>") + (nvsOku("ghtoken").length() > 0 ? "Kayitli" : "Yok") + "</p><form action=/ayar method=post><input type=password name=ghtoken placeholder=anahtar style='width:100%'><button>Kaydet</button></form>";
  h += "<h2>Alarmlar</h2><div class=k>" + htmlKacir(alarmListesi()) + "</div><form action=/sil><input type=hidden name=ne value=alarm><button>Alarmlari sil</button></form>";
  h += "</body></html>";
  webSunucu.send(200, "text/html; charset=utf-8", h);
}

void webAyar() {
  if (webSunucu.hasArg("ses")) {
    int v = webSunucu.arg("ses").toInt();
    if (v < 10) v = 10;
    if (v > 90) v = 90;
    gSesAyar = v;
    nvsYaz("ses", String(v));
  }
  if (webSunucu.hasArg("ghtoken")) { String t = webSunucu.arg("ghtoken"); t.trim(); if (t.length() >= 20 && t.length() < 200) nvsYaz("ghtoken", t); }
  if (webSunucu.hasArg("tarz")) nvsYaz("kisilik", temizMetin(webSunucu.arg("tarz"), 20));
  webSunucu.sendHeader("Location", "/");
  webSunucu.send(303);
}

void webSil() {
  String ne = webSunucu.arg("ne");
  if (ne == "hafiza") nvsYaz("hafiza", "");
  else if (ne == "not") notTemizle();
  else if (ne == "liste") { nvsYaz("l_alis", ""); nvsYaz("l_yap", ""); }
  else if (ne == "alarm") alarmTemizle();
  webSunucu.sendHeader("Location", "/");
  webSunucu.send(303);
}

void webGorevi(void* p) {
  while (true) {
    webSunucu.handleClient();
    vTaskDelay(8 / portTICK_PERIOD_MS);
  }
}

void webBaslat() {
  if (WiFi.status() != WL_CONNECTED) return;
  String sv = nvsOku("ses");
  if (sv.length() > 0) { int v = sv.toInt(); if (v >= 10 && v <= 90) gSesAyar = v; }
  MDNS.begin("finduk");
  webSunucu.on("/", webAna);
  webSunucu.on("/ayar", webAyar);
  webSunucu.on("/sil", webSil);
  webSunucu.begin();
  xTaskCreate(webGorevi, "web", 8192, NULL, 1, NULL);
  Serial.println(String("Ayar sayfasi: http://finduk.local veya http://") + WiFi.localIP().toString());
}
// ===== TELEFON AYAR SAYFASI, BILDIRIM BITIS =====

// ===== MOLA, PLAN, DINI GUNLER BASLA =====
volatile int gMolaDk = 0;
unsigned long gMolaSon = 0;

void molaYukle() {
  String v = nvsOku("mola");
  gMolaDk = v.length() ? v.toInt() : 0;
  if (gMolaDk < 0 || gMolaDk > 480) gMolaDk = 0;
  gMolaSon = millis();
}

void molaKur(const String& ar) {
  int dk = ar.toInt();
  if (dk < 0) dk = 0;
  if (dk > 480) dk = 480;
  if (dk > 0 && dk < 15) dk = 15;
  gMolaDk = dk;
  gMolaSon = millis();
  nvsYaz("mola", String(dk));
  Serial.println(String("MOLA hatirlatma: ") + dk + " dk");
}

bool molaVadesi() {
  if (gMolaDk <= 0 || gUyku) return false;
  long t = simdiEpoch();
  if (t == 0) return false;
  time_t tt = (time_t)t;
  struct tm z;
  localtime_r(&tt, &z);
  if (z.tm_hour < 8 || z.tm_hour >= 22) return false;
  return millis() - gMolaSon > (unsigned long)gMolaDk * 60000UL;
}

void molaCal() {
  gMolaSon = millis();
  Serial.println("MOLA vakti");
  gOzelBitis = millis() + 6000;
  gOzel = 3;
  gMood = M_MUTLU;
  gMoodBitis = millis() + 4000;
  i2s_set_sample_rates(I2S_NUM_1, 24000);
  gBipYuzde = SES_YUZDE;
  bip(1047, 140); sus(80); bip(1319, 180);
  sus(250);
  gDurum = D_KONUS;
  if (WiFi.status() == WL_CONNECTED) konus("Mola vakti! Biraz su ic, kalk ve esne.");
  gDurum = D_BOSTA;
}

// Haftalik plan (satir satir): "Pazartesi: dis hekimi saat 15"
String planAl() { return satirlariAl("plan"); }

bool planMi(const String& sd) {
  const char* k[] = {"plan", "program", "bu hafta", "haftalik", "bugun ne var", "randevu"};
  for (int i = 0; i < 6; i++) if (sd.indexOf(k[i]) >= 0) return true;
  return false;
}

// Hicri tarih (Aladhan). Kandil, bayram, Ramazan icin
String hicriBilgisi() {
  long t = simdiEpoch();
  if (t == 0) return "";
  time_t tt = (time_t)t;
  struct tm z;
  localtime_r(&tt, &z);
  char yol[64];
  snprintf(yol, sizeof(yol), "/v1/gToH/%02d-%02d-%04d", z.tm_mday, z.tm_mon + 1, z.tm_year + 1900);
  String r = httpsGetGovde("api.aladhan.com", yol, 4000);
  int k = r.indexOf("\"hijri\"");
  if (k < 0) return "";
  String seg = r.substring(k);
  int g = seg.indexOf("\"gregorian\"");
  if (g > 0) seg = seg.substring(0, g);
  String gun = degerBul(seg, "day");
  String ay = sayiBul(seg, "number");
  String yil = degerBul(seg, "year");
  if (gun.length() == 0 || ay.length() == 0) return "";
  const char* adlar[] = {"", "Muharrem", "Safer", "Rebiulevvel", "Rebiulahir", "Cemaziyelevvel", "Cemaziyelahir", "Recep", "Saban", "Ramazan", "Sevval", "Zilkade", "Zilhicce"};
  int an = ay.toInt();
  if (an < 1 || an > 12) return "";
  return String("Bugun hicri takvimle ") + gun + " " + adlar[an] + " " + yil + ". Onemli dini gunler (hicri): Hicri yilbasi 1 Muharrem, Asure 10 Muharrem, Mevlid Kandili 11 Rebiulevvel gecesi (12'sine baglanan gece), "
         "Uc aylarin baslangici 1 Recep, Regaib Kandili Recep'in ilk persembesini cumaya baglayan gece, Mirac Kandili 26 Recep gecesi, Berat Kandili 14 Saban gecesi, Ramazan baslangici 1 Ramazan, "
         "Kadir Gecesi 26 Ramazan gecesi, Ramazan Bayrami 1-3 Sevval, Kurban Bayrami 10-13 Zilhicce, Arefe bayramdan bir onceki gun. "
         "Bu hesap Diyanet takviminden bir gun sapabilir, kesin tarih icin Diyanet'e bakilmali.";
}

bool dinMi(const String& sd) {
  const char* k[] = {"kandil", "bayram", "ramazan", "hicri", "mubarek", "dini gun", "asure", "kurban", "regaip", "berat", "kadir gece", "mirac", "mevlid", "uc aylar"};
  for (int i = 0; i < 14; i++) if (sd.indexOf(k[i]) >= 0) return true;
  return false;
}
// ===== MOLA, PLAN, DINI GUNLER BITIS =====

// ===== LINK, GERI SAYIM BASLA =====
String urlKodla(const String& m) {
  String o;
  const char* h = "0123456789ABCDEF";
  for (unsigned int i = 0; i < m.length(); i++) {
    unsigned char ch = (unsigned char)m[i];
    if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')) o += (char)ch;
    else if (ch == ' ') o += '+';
    else { o += '%'; o += h[ch >> 4]; o += h[ch & 15]; }
  }
  return o;
}

// Telefona tiklanabilir bildirim (ntfy Click basligi)
void bildirLink(const String& baslik, const String& url) {
  if (WiFi.status() != WL_CONNECTED) return;
  WiFiClientSecure c;
  c.setInsecure();
  HTTPClient http;
  http.setTimeout(6000);
  if (!http.begin(c, String("https://ntfy.sh/") + SINEMA_KONU + "-robot")) return;
  http.addHeader("Content-Type", "text/plain; charset=utf-8");
  http.addHeader("Title", "Finduk");
  http.addHeader("Click", url);
  String govde = baslik + " - dokununca acilir";
  int kod = http.POST((uint8_t*)govde.c_str(), govde.length());
  Serial.println(String("Link bildirimi: ") + url + " -> HTTP " + kod);
  http.end();
}

// [link:trendyol|erkek kot pantolon] [link:hepsiburada|...] [link:amazon|...] [link:google|...]
void linkGonder(const String& ar) {
  int c = ar.indexOf('|');
  if (c < 0) return;
  String site = ar.substring(0, c);
  String q = ar.substring(c + 1);
  site.trim(); q.trim();
  if (q.length() == 0) return;
  String e = urlKodla(q);
  String url;
  if (site == "trendyol") url = String("https://www.trendyol.com/sr?q=") + e + "&sst=MOST_RATED";
  else if (site == "hepsiburada") url = String("https://www.hepsiburada.com/ara?q=") + e + "&siralama=yorumsayisi";
  else if (site == "amazon") url = String("https://www.amazon.com.tr/s?k=") + e + "&s=review-rank";
  else url = String("https://www.google.com/search?q=") + e;
  bildirLink(q, url);
}

// Geri sayim: satir bicimi "YYYY-AA-GG ad" ([geri:2027-02-16|ad] etiketindeki | kayitta bosluga doner)
void geriSayimEkle(const String& ar) { satirEkle("geri", ar, 15); }

String geriSayimlar() {
  String liste = nvsOku("geri");
  if (liste.length() == 0) return "Kayitli geri sayim yok.";
  long now = simdiEpoch();
  if (now == 0) return liste;
  String sonuc;
  int bas = 0;
  while (bas < (int)liste.length()) {
    int son = liste.indexOf('\n', bas);
    if (son < 0) son = liste.length();
    String satir = liste.substring(bas, son);
    bas = son + 1;
    satir.trim();
    int c = 10;
    if ((int)satir.length() < 12) continue;
    struct tm t = {};
    t.tm_year = satir.substring(0, 4).toInt() - 1900;
    t.tm_mon = satir.substring(5, 7).toInt() - 1;
    t.tm_mday = satir.substring(8, 10).toInt();
    t.tm_hour = 12;
    time_t hedef = mktime(&t);
    long gun = (long)((hedef - (time_t)now) / 86400);
    sonuc += satir.substring(c + 1) + ": " + (gun >= 0 ? String(gun) + " gun kaldi" : String(-gun) + " gun once oldu") + ". ";
  }
  return sonuc;
}

bool geriMi(const String& sd) {
  return sd.indexOf("kac gun kaldi") >= 0 || sd.indexOf("ne kadar kaldi") >= 0 || sd.indexOf("geri sayim") >= 0 || sd.indexOf("kac gun var") >= 0;
}
// ===== SUPURGE (Roborock Q8+, GitHub Actions koprusu) =====
// [supurge:baslat|durdur|sarj|bul|durum]  Anahtar finduk.local sayfasindan NVS'ye kaydedilir
bool supurgeGonder(const String& komut) {
  if (komut != "baslat" && komut != "durdur" && komut != "sarj" && komut != "bul" && komut != "durum") return false;
  String tok = nvsOku("ghtoken");
  if (tok.length() < 20 || WiFi.status() != WL_CONNECTED) { Serial.println("SUPURGE: anahtar yok veya ag yok"); return false; }
  WiFiClientSecure c;
  c.setInsecure();
  HTTPClient http;
  http.setTimeout(8000);
  if (!http.begin(c, "https://api.github.com/repos/okuzey3-cmyk/finduk/actions/workflows/supurge.yml/dispatches")) return false;
  http.addHeader("Authorization", String("Bearer ") + tok);
  http.addHeader("Accept", "application/vnd.github+json");
  http.addHeader("User-Agent", "finduk-robot");
  http.addHeader("Content-Type", "application/json");
  String govde = String("{\"ref\":\"main\",\"inputs\":{\"komut\":\"") + komut + "\"}}";
  int kod = http.POST((uint8_t*)govde.c_str(), govde.length());
  http.end();
  Serial.println(String("SUPURGE ") + komut + " -> HTTP " + kod);
  return kod == 204;
}

// ===== LINK, GERI SAYIM BITIS =====

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

static int gSesHiz = 24000;
static uint8_t sesTampon[4000];
static int sesN = 0;
static bool gSesSon = false;     // son parca mi (kuyrugu da yaz)
static bool gSesIlk = true;      // ilk parca mi (yumusak giris)
const int SES_TUT = 1600;        // sonda yumusak cikis icin tutulan bayt (yaklasik 33 ms)

void sesYaz() {
  int yaz_n = gSesSon ? (sesN / 2) * 2 : ((sesN - SES_TUT) / 2) * 2;
  if (yaz_n <= 0) return;
  int16_t* s = (int16_t*)sesTampon;
  if (gSesIlk) {                                   // yumusak giris (tik sesini onler)
    int m = yaz_n / 2; if (m > 480) m = 480;
    for (int i = 0; i < m; i++) s[i] = (int16_t)(((int32_t)s[i] * i) / m);
    gSesIlk = false;
  }
  if (gSesSon) {                                   // yumusak cikis (pirt sesini onler)
    int m = yaz_n / 2; if (m > 800) m = 800;
    for (int i = 0; i < m; i++) { int j = yaz_n / 2 - 1 - i; s[j] = (int16_t)(((int32_t)s[j] * i) / m); }
  }
  if (MAC_SES) {
    uint8_t h[8] = {0xFF, 0xAA, 'P', 'C', (uint8_t)(yaz_n & 255), (uint8_t)(yaz_n >> 8), (uint8_t)(gSesHiz & 255), (uint8_t)(gSesHiz >> 8)};
    Serial.write(h, 8);
    Serial.write(sesTampon, yaz_n);
  }
  long toplamMutlak = 0;
  for (int i = 0; i < yaz_n / 2; i++) {
    s[i] = (int16_t)(((int32_t)s[i] * (MAC_SES ? 0 : sesYuzdeSimdi())) / 100);
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
  if (sesN >= 3600) sesYaz();
}

bool konusAl(const String& metin, bool akis) {
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

  String baslik = String("POST /v1beta/models/") + TTS_MODEL + (akis ? ":streamGenerateContent?alt=sse" : ":generateContent") + " HTTP/1.1\r\n" +
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
        k = 0;
        if (!calindi) {
          int rate = 24000;
          int ri = bas.indexOf("rate=");
          if (ri >= 0) rate = bas.substring(ri + 5).toInt();
          if (rate < 8000 || rate > 96000) rate = 24000;
          gSesHiz = rate;
          i2s_set_sample_rates(I2S_NUM_1, rate);

          gDurum = D_KONUS;
          delay(30);
          gSesIlk = true;
          gSesSon = false;
          calindi = true;
        }
      }
    } else {
      if (b == '"') {
        if (k == 2) sesEkle((dortlu[0] << 2) | (dortlu[1] >> 4));
        if (k == 3) {
          sesEkle((dortlu[0] << 2) | (dortlu[1] >> 4));
          sesEkle(((dortlu[1] & 15) << 4) | (dortlu[2] >> 2));
        }
        k = 0;
        if (!akis) break;
        durumKod = 0;     // akista siradaki parcayi ara
        eslesen = 0;
        continue;
      }
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

  if (calindi) {
    gSesSon = true;
    sesYaz();
    gSesSon = false;
    {   // kuyrukta kalan sesin kesilmemesi icin biraz sessizlik yaz, sonra kapat
      static uint8_t sus[1024];
      memset(sus, 0, sizeof(sus));
      for (int i = 0; i < 20; i++) {
        size_t yz = 0;
        i2s_write(I2S_NUM_1, sus, sizeof(sus), &yz, portMAX_DELAY);
      }
    }
    i2s_zero_dma_buffer(I2S_NUM_1);
    delay(100);
    // amfi acik kalir (acma/kapama cizirtisini onlemek icin)
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

// Once akisli (parca parca, hizli baslar), olmazsa tek parca dener
bool konus(const String& metin) {
  if (konusAl(metin, true)) return true;
  Serial.println("(Akisli ses olmadi, normal deneniyor)");
  return konusAl(metin, false);
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
  zamanBaslat();
  alarmYukle();
  webBaslat();
  molaYukle();
  bildir(String("Finduk acildi. Surum ") + FW_SURUM + ". " + resetNedeni());
  Serial.println("Finduk: kablosuz guncelleme calisiyor.");
  guncellemeKontrol();
}

void loop() {
  Serial.println();
  Serial.println("Dinliyorum... (konusmaya basla)");
  gDurum = D_BOSTA;
  if (gUyku) {
    Serial.println("Uyuyorum... (bir ses cikar, uyanayim)");
    if (!bekleSes(GUNCEL_ARALIK, ESIK_UYKU)) {
      if (alarmCal()) return;
      guncellemeKontrol();      // uyurken yeni surum var mi diye bak
      return;
    }
    gUyku = false;
    Serial.println("Uyandim! Simdi konusabilirsin.");
    delay(1800);                 // uyanma animasyonu
    sabahSelami();
    return;
  }
  if (!bekleSes(UYKU_SURE, ESIK)) {
    if (alarmCal()) return;
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
  bool nm = (!hv && !hb && namazMi(sd));
  if (nm) {
    Serial.println("(Namaz vakitleri aliniyor...)");
    bilgi = namazBilgisi();
  }
  bool fn = (!hv && !hb && !nm && finansMi(sd));
  if (fn) {
    Serial.println("(Kurlar aliniyor...)");
    bilgi = finansBilgisi();
  }
  bool tr = (!hv && !hb && !nm && !fn && trafikMi(sd));
  if (tr) {
    Serial.println("(Trafik bilgisi aliniyor...)");
    bilgi = trafikBilgisi();
  }
  bool mc = (!hv && !hb && !nm && !fn && !tr && macMi(sd));
  if (mc) {
    Serial.println("(Mac bilgisi aliniyor...)");
    bilgi = macBilgisi();
  }
  if (hv) {
    Serial.println("(Hava bilgisi aliniyor...)");
    bilgi = havaBilgisi();
  } else if (hb) {
    Serial.println("(Haberler aliniyor...)");
    bilgi = haberBilgisi();
  }
  if ((hv || hb || nm || fn || tr || mc) && bilgi.length() == 0) {
    bilgi = "Veri su an cekilemedi. Kullaniciya su an bakamadigini kisaca soyle.";
    Serial.println("(Bilgi alinamadi)");
  } else if (bilgi.length() > 0) {
    Serial.print("BILGI: ");
    Serial.println(bilgi);
  }

  if (!hv && !hb && !nm && !fn && !tr && !mc && sinemaMi(sd)) {
    String d = sinemaDurum();
    if (d.length() > 0) {
      bilgi = String("Demet'in Sinema Odasi uygulamasinin son bilinen ekrani: ") + d + " (Bu bilgi sadece durumu gosterir. Istenirse komutla kontrol edebilirsin.)";
      Serial.print("SINEMA DURUM: ");
      Serial.println(d);
    }
  }

  {
    String ek = "";
    if (notMi(sd)) {
      String nl = notlariAl();
      ek += String("Kayitli notlar: ") + (nl.length() ? nl : String("(yok)")) + ". ";
    }
    if (alarmMi(sd)) {
      String al = alarmListesi();
      ek += String("Kurulu alarmlar: ") + (al.length() ? al : String("(yok)")) + ". ";
    }
    if (geriMi(sd)) ek += String("Geri sayimlar: ") + geriSayimlar() + " ";
    if (planMi(sd)) { String pl = planAl(); ek += String("Kayitli plan: ") + (pl.length() ? pl : String("(bos)")) + ". "; }
    if (dinMi(sd)) ek += hicriBilgisi() + " ";
    if (haftalikMi(sd)) ek += haftalikHava() + " ";
    if (listeMi(sd)) ek += listeleriAl() + " ";
    if (ozetMi(sd)) {
      Serial.println("(Gun ozeti hazirlaniyor...)");
      String nb = namazBilgisi();
      ek += String("GUN OZETI icin: ") + havaBilgisi() + " " + nb + " Plan: " + planAl() + ". Notlar: " + notlariAl() + ". Alarmlar: " + alarmListesi() + ". " + listeleriAl() + " ";
    }
    if (ek.length() > 0) bilgi = (bilgi.length() > 0) ? bilgi + " " + ek : ek;
  }

  String ham = claudeCevapla(duyulan, bilgi);
  String cevap = degerBul(ham, "text");
  if (cevap.length() > 0) {
    String hamCevap = cevap;          // etiketli hali hafiza icin
    String komutlar[6];
    int komutN = komutlariAyikla(cevap, komutlar, 6);
    int mood = moodCoz(cevap);        // etiketi ayiklar
    Serial.print("FINDUK (");
    Serial.print(mood);
    Serial.print("): ");
    Serial.println(cevap);
    hafizayaEkle(duyulan, hamCevap);
    gMood = mood;
    konus(cevap);
    for (int i = 0; i < komutN; i++) {      // konusma bitince uygulamaya komutlari yolla
      komutIsle(komutlar[i]);
      delay(300);
    }
    gMoodBitis = millis() + 2500;
    sohbetBitis = millis() + SOHBET_SURE;     // 30 sn boyunca adini soylemeden devam edebilirsin
  } else {
    Serial.println("Claude cevap veremedi. Ham cevap:");
    Serial.println(ham.length() > 700 ? ham.substring(ham.length() - 700) : ham);
  }
  gDurum = D_BOSTA;
  delay(1000);
}
