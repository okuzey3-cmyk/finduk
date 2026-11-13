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
#include <WiFiUdp.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include "sdkconfig.h"
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
  "filtre|alan=deger (alanlar: kategori film veya dizi veya turk veya yabanci, tur, sure, kimle aile veya cocuk, platform, yil recent veya 2020 veya 2010 veya 2000 veya 1990 veya 1980 veya old, puan 6 veya 7 veya 8; birden fazlasi icin ; ile ayir ornek filtre|tur=Komedi;yil=2010); filtretemizle; profil|demet veya oguzhan; yakindasec|1 (Yakinda ekranindaki n. filmi acar); ajandaac|1 (ajandadaki n. satiri acar); sahnesec|Ankara veya Izmir veya konser veya tiyatro (Sahne ekraninda sehir ya da tur secer); rafsec|1 (Rafim ekranindaki n. filmi acar); rafara|metin (rafta film adi arar, bos birakirsan aramayi temizler); rafsirala|yeni veya puan veya yil veya ad (raf siralamasi); rafmenu|biz veya atolye veya pasaport veya gunluk veya yil veya gorev veya rozet (Rafim ekranindaki bolumleri acar); neizlesem|ruh;sure;kim (3 soruda film sec penceresini acip cevaplar; ruh: gul veya duy veya ger veya kafa veya aks veya haf, sure: kisa veya orta veya fark veya dizi, kim: tek veya cift veya aile veya arkadas; ornek neizlesem|gul;kisa;cift; eksik yazarsan sadece o kadar adim ilerler); nebaska (ayni sorularla baska 6 film); nebastan (sorulara bastan basla); ovsec|1 (acik penceredeki n. filmi acar); filmgecesi (acik biletin film gecesi menusunu acar); kadifesor (acik yapimi Kadife'ye sorar); eslestir|ayni film veya eslestir|ayni dizi (birlikte film eslestirme penceresini ayni telefon modunda acar; sadece eslestir yazarsan mod secimine kadar acar); begen ve gec (eslestirme kartini begenir ya da gecer); oyla|izleyecek veya oyla|kadife (3 aday oylama penceresini acar; adaylari ovsec|n ile isaretle; WhatsApp gonderimini sen yapmazsin, kullanici kendisi basar); kadifeses|ac veya kadifeses|kapat (uygulamada Kadife sesli yanit); salon|netflix,hbo (Kesfet ekranindaki Salonlarim: sadece yazdigin platformlari acik birakir; salon|hepsi hepsini acar; salon|+hbo bir platformu acar, salon|-prime kapatir; adlar netflix prime disney tv hbo apple); salonyeni|ac veya salonyeni|kapat (sadece son 1 yilin yapimlari modu); paketsec|1 (Paketinde yeni cikanlar seridindeki n. yapimi acar); zevksec|1 (Senin zevkine gore seridindeki n. yapimi acar); kadifeayar (Kadife ekraninda ayarlar kismini acar, sadece gosterir); geri; kapat; kaydir|asagi veya yukari; yenile; dugme|ekrandaki dugmenin yazisi. "
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
  " KENDILIGINDEN KONUSMA VE HUY: Bazen sessizlikte kendin konusursun, bu sana ayrica talimat olarak verilir. Gunluk huyun sana bildirilir. "
  " ORTAK ESPRI: Kullanici ikinizin ortak bir espri, lakap veya guzel bir ani olusturdugunu soylerse ya da komik bir an yasanirsa [espri:kisa metin] yaz (en fazla 90 karakter); silmek icin [esprisil:parca]. "
  " SESLI MACERA: Kullanici macera oyunu isterse [macera:basla|tema] yaz ve hikayeyi anlatmaya basla. Surerken [macera:ozet|guncel ozet] yaz. Bitince [macera:bitir] yaz. "
  " KARAKTER MODU: Kullanici bir karakter isterse (ornek Osmanli taciri) [karakter:karakterin kisa tarifi] yaz ve o agizla konus. Cikmak icin [karakter:kapat]. "
  " TARTISMA: Kullanici tartisalim derse karsi tarafi savun, kisa ve esprili itiraz et, inatci ama saygili ol, gercekten ikna olursan kabul et. "
  " GUNUN TEKERLEMESI: Kullanici gunun tekerlemesini, siirini veya sarkisini isterse bugunku konusmalara ve guzel anlara dayanan kisa, kafiyeli, komik, 4 satirlik OZGUN bir tekerleme yaz. Var olan sarki veya siirlerden alinti yapma. "
  " KIM KONUSUYOR: Konusan Demet oldugunu soylerse [kisi:demet], Oguzhan ise [kisi:oguzhan] yaz. Demet konusurken yazdigin hafiza komutlari Demet'in hafizasina gider. "
  " RUH HALI: Kullanicinin sozlerinden yorgun, uzgun, stresli veya neseli oldugunu sezersen tavrini ayarla: yorgunsa kisa ve sakin konus, uzgunse once dinle ve destek ol. Ciddi bir sikinti sezersen yakinlarindan destek almasini nazikce oner. Tibbi teshis koyma. "
  " INGILIZCE PRATIK: Kullanici Ingilizce pratik yapmak isterse [dil:ingilizce|seviye] yaz, bitirmek icin [dil:kapat]. "
  " TARIF ADIM ADIM: Kullanici bir yemegi birlikte yapmak isterse tarifi en fazla 8 kisa adim halinde [tarif:yemek adi|adim bir;adim iki;adim uc] yaz (noktali virgul ile ayir). Robot adimlari sonraki, tekrar, onceki denince kendisi okur. Bekleme gereken adimlar icin ayrica [zamanlayici:...] yazabilirsin. "
  " VARDIYA: Kullanici vardiya duzenini soylerse (ornek haftalik donen sabah, aksam, gece) [vardiya:ilk haftanin pazartesi tarihi YYYY-AA-GG|sabah,aksam,gece] yaz, siradaki haftalar tekrar eder. Vardiya degisimlerinde uyku duzeni icin kisa, genel oneriler ver (tibbi tavsiye degil). Kapatmak icin [vardiya:kapat]. "
  " SU VE MOLA: Su icme hatirlaticisi istenirse [su:aralik dakika|baslangic saati|bitis saati] yaz, kapatmak icin [su:kapat]. "
  " ARAC BAKIMI: Muayene, sigorta, bakim gibi tarihler icin [arac:ad|YYYY-AA-GG] yaz, silmek icin [aracsil:ad]. Tarih yaklasinca telefona bildirim gider. "
  " YEMEK PLANI: Haftalik yemek plani istenirse kullanicinin zevklerine gore hazirla, [yemekplan:Pzt corba makarna;Sal ...] ile kaydet (gunleri noktali virgulle ayir), ayrintiyi [telefon:...] ile gonder. Silmek icin [yemekplansil:hepsi]. "
  " HEDIYE VE SURPRIZ: Yildonumu veya dogum gunu icin hediye ya da surpriz isterse kayitli ozel gunleri ve hafizadaki zevkleri kullan, 3 somut fikir ver, butceyi sor. "
  " FILM AKSAMI: Film izleyelim denince once kimlerin izleyecegini ve ruh halini tek soruyla ogren. Iki kisi farkli sey isterse ortak nokta bul, 2 secenek sun, secilince Sinema Odasi komutlariyla ac. "
  " GIYIM: Ne giyeyim sorulursa sana verilen hava bilgisine gore somut oner. "
  " UZUN CEVAPLAR: Seyahat plani gibi uzun seyleri sesli kisaca anlat, ayrintiyi [telefon:ilk kisim] ve [telefon:ikinci kisim] seklinde telefona gonder. "
  " MAC GUNU: Kullanici bir macin takibini isterse [mac:takim adi|SS:DD] yaz, tarih bugun degilse [mac:takim adi|SS:DD|YYYY-AA-GG]. "
  " GIZLI KASA: Kullanici sesli parolayla saklanacak ozel bir not verirse [kasa:tekkelimeparola|not] yaz (parola en az 4 harf, tek kelime). Notu sen soyleme, kasayi robot parolayi duyunca kendisi acar. Silmek icin [kasasil:parola]. "
  " MISAFIR MODU: Misafir geldigini soylerse [misafir:ac], gidince [misafir:kapat] yaz. "
  " EV CIHAZLARI: Sana kayitli ev cihazi bildirilirse acmak veya kapatmak icin [cihaz:ad|ac] ya da [cihaz:ad|kapat] yaz. "
  " TELEVIZYON: Samsung televizyon icin [tv:tus|KEY_VOLUP] gibi yaz (KEY_POWER KEY_VOLUP KEY_VOLDOWN KEY_MUTE KEY_HOME KEY_RETURN KEY_ENTER KEY_UP KEY_DOWN KEY_LEFT KEY_RIGHT KEY_PLAY KEY_PAUSE KEY_FF KEY_REWIND KEY_STOP KEY_INFO KEY_MENU KEY_SOURCE KEY_GUIDE KEY_TOOLS). Kanal icin [tv:kanal|+] veya [tv:kanal|-] ya da numara [tv:kanal|35]. TV kapaliysa [tv:ac|1] ile uyandirabilirsin (uygulama, film ve kanal komutlari da kapali TV'yi kendisi uyandirir, bu 40 saniyeye kadar surebilir, kullaniciya soyle). Sinema Odasi uygulamasini televizyonda acmak icin [tv:web|sinema] yaz. Kullanici TV aramasini acip "yaz" derse [tv:yaz|metin] yaz (metni bos birakirsan son acilan film adi yazilir). Kullanici bir platform icin baska yontem denemeni isterse [tv:yontem|disney 2] yaz (1 tam adres, 2 sadece kimlik, 3 DIAL, 4 sadece uygulamayi ac). Uygulama acmak icin [tv:uygulama|platform adi] yaz (netflix, youtube, prime, disney, hbo max, blutv, exxen, tod, tabii, mubi gibi; TV de yukluyse acilir). Kullanici bir FILM veya DIZIYI televizyonda acmani isterse [tvfilm:filmin adi] yaz; robot filmin hangi platformda oldugunu kendisi bulur, aciyor ve sonucu kendisi soyler, sen sadece kisaca bakiyorum de, platform tahmin etme. Sinema Odasi favorilerinden birini acman istenirse sana verilen Sinema Odasi bilgisinde film adi varsa ondan sec, yoksa kullanicinin sevdigini bildigin bir film sec ve hangisini sectigini soyle. "
  " WIFI KURULUM: Kullanici yeni Wi-Fi ayarlamak isterse [kurulum:ac] yaz. Telefon Finduk-Kurulum agina baglanip 192.168.4.1/kurulum sayfasini acar. "
  "Dolar, euro, altin sorulursa sana verilen kurlari soyle, yorum yapma, tahmin etme. Mac, takim veya baska guncel konu sorulursa sana verilen bilgi yoksa bilmedigini soyle. Ezan vaktinde hatirlatma istenirse sana verilen vakitle alarm kur. MOLA: Kullanici belirli araliklarla mola hatirlatmasi isterse [mola:dakika] yaz (en az 15), kapatmak icin [mola:0]. PLAN: Kullanici bir gun icin plan ya da randevu soylerse [plan:Pazartesi saat 15 dis hekimi] yaz, silmek icin [plansil:dis hekimi] veya [plansil:hepsi]; bugunun gunune bak ve plani sorulunca soyle. Dini gun, kandil ya da bayram sorulursa sana verilen hicri tarih bilgisini kullan, tarih konusunda kesin konusma, Diyanet'e bakmasini hatirlat. GOZLER: Oyunda kullanici kazanirsa veya sevincli bir an olursa [goz:kutla], oyun baslarken [goz:oyun], saat gosterilmesini isterse [goz:saat] yaz. TELEFON: Kullanici bir urunun linkini telefonuna istersa [link:trendyol|erkek kot pantolon] yaz (site: trendyol, hepsiburada, amazon veya google; arama ifadesi kisa ve net, puana gore siralanir), sana belirli bir urun secme, telefonda aramayi acan link gider; kisa bir mesaj istenirse [telefon:mesaj] yaz. SUPURGE: Kullanici robot supurgeyi baslatmak, durdurmak, sarja gondermek veya bulmak isterse [supurge:baslat], [supurge:durdur], [supurge:sarj], [supurge:bul] yaz ve kisa onayla; komut yaklasik bir dakikada ulasir, hemen basladi deme, komutu gonderdim de. IYI GECELER: Kullanici iyi geceler derse sicak ve kisa cevap ver, yarinin havasini tek cumleyle soyle, kayitli alarm yoksa yarin icin alarm isteyip istemedigini sor; alarm zaten varsa saatini hatirlat ve sonuna [uyu:1] yaz. Kullanici alarm saatini soylerse alarmi kur ve [uyu:1] ekle. CEVIRI: Kullanici bir seyi cevirmeni veya ne demek oldugunu sorarsa ceviriyi soyle, yabanci cumleyi yavas ve net soyle, Turkce okunusunu da ekle, sonuna [telefon:asil metin = ceviri] yaz ki telefonuna da gitsin. FIYAT ALARMI: Kullanici dolar, euro veya gram altin belli bir fiyata gelince haber istenirse [fiyat:dolar|>|45] yaz (yukari icin >, asagi icin <; varlik dolar, euro veya altin), Finduk yarim saatte bir bakip telefonuna bildirir; iptal icin [fiyatsil:dolar] veya [fiyatsil:hepsi]. Yatirim tavsiyesi verme. TELEFONUMU BUL: Kullanici telefonunu bulamazsa [telefonbul:1] yaz, telefonuna sesli acil bildirim gider (sessizdeyse duyulmayabilir). ALISKANLIK: Kullanici takip etmek istedigi bir aliskanlik soylerse [aliskanlik:su icmek] yaz, yaptigini soylerse [yaptim:su icmek] yaz (ad tanimdakiyle ayni olsun), istemezse [aliskanliksil:ad]; seri veya bu hafta sorulursa sana verilen kayitlardan kac gun ust uste yapildigini hesapla, kisa ve cesaretlendirici ol, dirdir etme. HIKAYE: Kullanici bir hikaye, ani veya tarif kaydetmek isterse kisa bir baslik sec, [hikaye:baslik] yaz ve anlatmaya baslamasini soyle; bundan sonra anlattiklari oldugu gibi kaydedilir, bitince hikaye bitti demesi yeterli, bunu da soyle. GUN OZETI: Her aksam 21.30da telefona gun ozeti gider; kullanici kapatmak isterse [ozet:kapat], acmak isterse [ozet:ac], simdi isterse [ozet:simdi] yaz. TELEFONDAN YAZMA: Kullanici telefondan da yazabilir; mesajlari birkac dakikada bir okunur. OZEL GUNLER: Her yil tekrar eden dogum gunu, yildonumu gibi gunleri [ozelgun:02-16|evlilik yildonumu] diye kaydet (ay-gun), silmek icin [ozelgunsil:ad]; yedi gun once, bir gun once ve o gun telefona hatirlatilir; bugun ozel gun ise sohbette sicakca hatirlat ve hediye veya kutlama fikri ver. TEKRARLAYAN HATIRLATICI: Belli gun ve saatte tekrar eden hatirlatma icin [hatirlat:19:00|pzt,per|cop cikacak] yaz; gunler hergun, haftaici, haftasonu veya pzt,sal,car,per,cum,cmt,paz (bosluksuz, saat iki haneli); silmek icin [hatirlatsil:mesajdan bir kelime] ya da [hatirlatsil:hepsi]. GUZEL ANLAR: Iyi geceler denince bazen bugunun guzel bir anini sor; kullanici guzel bir an, sevindigi veya sukrettigi bir sey anlatirsa [gunluk:kisa ozet] yaz; pazar aksami haftanin anlari telefona gider. EMANET: Biri evdeki baskasina mesaj birakmak isterse [emanet:Demet|mesaj] yaz; bekleyen emanet varsa ve konusan kisi o kisiyse veya kim oldugunu soylerse mesaji ilet ve [emanetsil:Demet] yaz; kime ait oldugu belli degilse kime mesaj oldugunu soyle, icerigi sahibine sakla. GERI SAYIM: Kullanici onemli bir tarih soylerse [geri:2027-02-16|evlilik yildonumu] yaz, silmek icin [gerisil:ad] veya [gerisil:hepsi]; kac gun kaldi sorulursa sana verilen sayimi soyle. Gun ozeti istenirse sana verilen saat, hava, namaz, alarm, not ve listeleri kisa ve duzenli soyle.";

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
bool gSesYeniBasladi = true;   // ses filtresi sifirlama
unsigned long gSonFiyat = 0;    // fiyat alarmi son kontrol
String gHikayeBaslik = "";      // hikaye kayit modu acikken baslik
String gHikayeMetin = "";
String gYazSon = "";            // telefondan gelen son mesaj kimligi
unsigned long gSonYazKontrol = 0;
long gSonHatirlat = 0;          // tekrarlayan hatirlatici son kontrol zamani
// --- Paket 28 globalleri ---
String gKisi = "";                 // konusan kisi: bos = Oguzhan, "demet" = Demet
unsigned long gKisiBitis = 0;
bool gMisafir = false;
unsigned long gMisafirBitis = 0;
unsigned long gSonEtkilesim = 0;   // son sohbet (millis)
unsigned long gSonKendi = 0;       // son kendiliginden konusma (millis)
unsigned long gSonSu = 0;
bool gKonustu = false;             // periyodik is konustu mu
bool gWebVar = false;              // web gorevi calisiyor mu
String gTarifBaslik = "";
String gTarifAdim[12];
int gTarifN = 0;
int gTarifI = 0;
// --- Paket 29 globalleri ---
bool gGeriDogrulandi = false;
unsigned long gMasalBitis = 0;     // uyku masali: ses kisik
bool gNobet = false;
unsigned long gNobetBitis = 0;
unsigned long gNobetSon = 0;
int gNobetSay = 0;
int gNobetStt = 0;
int gNobetEsik = 9000;

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
  if (yeni <= nvsOku("kotu_s").toInt()) return;   // bozuk cikan surumu tekrar indirme

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
    geriDonDogrula();
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

  String onek = String("{\"contents\":[{\"parts\":[{\"text\":\"") + (nvsOku("dil_modu").length() ? "Bu ses kaydini aynen yaziya cevir. Konusma Turkce ya da Ingilizce olabilir, hangi dildeyse o dilde yaz." : "Bu ses kaydini Turkce olarak aynen yaziya cevir.") + String(" Konusmada Finduk adinda bir robota seslenilebilir, bu ismi Findik diye degil aynen Finduk diye yaz. Sadece soylenen cumleyi yaz, baska hicbir sey ekleme. Ses yoksa BOS yaz.\"},{\"inline_data\":{\"mime_type\":\"audio/wav\",\"data\":\"");
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
  { String hf = satirlariAl(gKisi == "demet" ? "hafiza_demet" : "hafiza"); if (!gMisafir && hf.length()) sistem += String(gKisi == "demet" ? " Konusan kisi Demet, onun hakkinda hafizandaki bilgiler: " : " Kullanici hakkinda hafizandaki bilgiler: ") + hf + ".";
    String kt = nvsOku("kisilik"); if (!gMisafir && kt.length()) sistem += String(" Konusma tarzin: ") + kt + ".";
    if (sesYuzdeSimdi() < gSesAyar) sistem += " Gece oldu, sakin ve kisa konus.";
    sistem += paket28Sistem(); }
  if (bilgi.length() > 0) {
    sistem += " ASAGIDA INTERNETTEN ALINMIS GUNCEL BILGI VAR. Cevabini buna dayandir. Haber ise en fazla 3 basligi kisa kisa soyle, hava ise sicakligi ve durumu soyle. Bilgi: " + bilgi;
  }
  String govde = String("{\"model\":\"") + CLAUDE_MODEL + "\",\"max_tokens\":450,\"system\":\"" +
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

// Havalimani resmi hava raporu: METAR (anlik) ve TAF (tahmin). Kaynak NOAA Aviation Weather Center, Turk Meteoroloji verisini yayinlar
String metarBilgisi(bool taf) {
  String m = httpsGetGovde("aviationweather.gov", "/api/data/metar?ids=LTFM,LTFJ&format=raw", 900);
  m.trim();
  if (m.length() < 20) return "";
  String o = String("RESMI HAVACILIK RAPORU (METAR, LTFM Istanbul Havalimani ve LTFJ Sabiha Gokcen): ") + m + " ";
  if (taf) {
    String t = httpsGetGovde("aviationweather.gov", "/api/data/taf?ids=LTFM&format=raw", 1200);
    t.trim();
    if (t.length() > 20) o += String("TAF tahmini (LTFM): ") + t + " ";
  }
  o += "Bu kodlari sade Turkce ile yorumla: ruzgar (dugum km/saate cevrilir, 1 dugum yaklasik 1.85 km/s), gorus mesafesi, bulut, sicaklik, yagis veya sis varsa. Kaynagi kisaca soyle: havalimani resmi METAR raporu. Kod okuma, ozetle.";
  return o;
}

bool havaMi(const String& sd) {
  const char* k[] = {"hava durum", "hava nasil", "kac derece", "sicaklik", "yagmur", "yagacak", "kar yag", "soguk mu", "sicak mi", "hava kac", "havalimani", "metar", "ruzgar", "sisli", "sis var", "ucuslar", "ne giy", "mont", "semsiye", "ceket", "kazak"};
  for (int i = 0; i < 21; i++) if (sd.indexOf(k[i]) >= 0) return true;
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
  const char* k[] = {"film", "bilet", "sinema", "dizi", "fragman", "zarf", "favori", "izle", "oyuncu", "puan", "raf", "kadife", "aday", "filtre", "ekran", "uygulama", "vizyon", "sahne", "ajanda", "yakinda", "pasaport", "rozet", "gunluk", "gorev", "esles", "oylama", "kartta", "secilen", "salon", "paketim", "zevk", "platform"};
  for (int i = 0; i < 32; i++) if (sd.indexOf(k[i]) >= 0) return true;
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
    if (ad == "sinema" || ad == "zamanlayici" || ad == "alarm" || ad == "alarmsil" || ad == "not" || ad == "notsil" || ad == "hafiza" || ad == "hafizasil" || ad == "liste" || ad == "kisilik" || ad == "goz" || ad == "mola" || ad == "plan" || ad == "plansil" || ad == "link" || ad == "supurge" || ad == "fiyat" || ad == "fiyatsil" || ad == "uyu" || ad == "telefonbul" || ad == "aliskanlik" || ad == "aliskanliksil" || ad == "yaptim" || ad == "hikaye" || ad == "ozet" || ad == "ozelgun" || ad == "ozelgunsil" || ad == "hatirlat" || ad == "hatirlatsil" || ad == "gunluk" || ad == "emanet" || ad == "emanetsil" || ad == "telefon" || ad == "geri" || ad == "gerisil" || ad == "espri" || ad == "esprisil" || ad == "macera" || ad == "karakter" || ad == "kisi" || ad == "misafir" || ad == "dil" || ad == "tarif" || ad == "vardiya" || ad == "su" || ad == "arac" || ad == "aracsil" || ad == "yemekplan" || ad == "yemekplansil" || ad == "kasa" || ad == "kasasil" || ad == "mac" || ad == "cihaz" || ad == "tv" || ad == "tvfilm" || ad == "kurulum" || ad == "radyo" || ad == "dolgu" || ad == "nobet" || ad == "takim") {
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
  else if (ad == "hafiza") satirEkle(gKisi == "demet" ? "hafiza_demet" : "hafiza", ar, 30);
  else if (ad == "hafizasil") { const char* hk = (gKisi == "demet") ? "hafiza_demet" : "hafiza"; if (ar == "hepsi") nvsYaz(hk, ""); else satirSil(hk, ar); }
  else if (ad == "liste") listeIsle(ar);
  else if (ad == "kisilik") nvsYaz("kisilik", temizMetin(ar, 20));
  else if (ad == "mola") molaKur(ar);
  else if (ad == "plan") satirEkle("plan", ar, 20);
  else if (ad == "link") linkGonder(ar);
  else if (ad == "fiyat") { satirEkle("fiyat", ar, 10); gSonFiyat = 0; }
  else if (ad == "fiyatsil") { if (ar == "hepsi") nvsYaz("fiyat", ""); else satirSil("fiyat", ar); }
  else if (ad == "uyu") gUyku = true;
  else if (ad == "telefonbul") telefonuBul();
  else if (ad == "ozelgun") satirEkle("ozelgun", ar, 30);
  else if (ad == "ozelgunsil") satirSil("ozelgun", ar);
  else if (ad == "hatirlat") { String h = ar; if (h.length() > 1 && h[1] == ':') h = String("0") + h; h.replace(", ", ","); satirEkle("hatirlat", h, 15); }
  else if (ad == "hatirlatsil") { if (ar == "hepsi") nvsYaz("hatirlat", ""); else satirSil("hatirlat", ar); }
  else if (ad == "gunluk") { satirEkle("gunluk", bugunTarih() + " " + ar, 60); aniKaydet(ar); }
  else if (ad == "emanet") { int c = ar.indexOf('|'); if (c > 0) satirEkle("emanet", ar.substring(0, c) + ": " + ar.substring(c + 1), 10); }
  else if (ad == "emanetsil") { if (ar == "hepsi") nvsYaz("emanet", ""); else satirSil("emanet", ar); }
  else if (ad == "aliskanlik") satirEkle("alsk_t", ar, 12);
  else if (ad == "aliskanliksil") satirSil("alsk_t", ar);
  else if (ad == "yaptim") aliskanlikYaptim(ar);
  else if (ad == "hikaye") { hikayeBitir(); gHikayeBaslik = temizMetin(ar, 60); gHikayeMetin = ""; }
  else if (ad == "ozet") { if (ar == "kapat") nvsYaz("ozet_kapali", "1"); else if (ar == "ac") nvsYaz("ozet_kapali", ""); else { nvsYaz("ozet_gun", ""); bildirBaslik("Finduk gun ozeti", String("Plan: ") + planAl() + "\nNotlar: " + notlariAl() + "\nAlisveris: " + satirlariAl("l_alis") + "\nYapilacaklar: " + satirlariAl("l_yap"), 3, "memo"); } }
  else if (ad == "supurge") { if (!supurgeGonder(ar)) bildir("Supurge komutu gonderilemedi (anahtar eksik olabilir)"); }
  else if (ad == "telefon") bildir(ar);
  else if (ad == "geri") geriSayimEkle(ar);
  else if (ad == "gerisil") { if (ar == "hepsi") nvsYaz("geri", ""); else satirSil("geri", ar); }
  else if (ad == "plansil") { if (ar == "hepsi") nvsYaz("plan", ""); else satirSil("plan", ar); }
  else if (paket28Komut(ad, ar)) { }
  else if (paket29Komut(ad, ar)) { }
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
  String bilgi = String("Bugun ozel gun varsa (") + ozelGunler(0) + ") onu da sicakca hatirlat. Bekleyen emanet mesaj varsa (" + emanetler() + ") kime ait oldugunu soyle. Sabah selami ver: gunaydin de, havayi ve gunun bir iki onemli haberini cok kisa soyle. Kayitli plan varsa bugune ait olani da soyle. Plan: " + planAl() + ". " + havaBilgisi() + " " + haberBilgisi();
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
  if (millis() < gMasalBitis) { int q = v * 40 / 100; return q > 8 ? q : 8; }   // uyku masali: fisilti
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
  h += "<h2>Wi-Fi degistir</h2><form action=/ayar method=post><input name=wifiad placeholder='Ag adi' style='width:100%'><input type=password name=wifisifre placeholder='Sifre' style='width:100%'><button>Kaydet ve yeniden baslat</button></form>";
  h += String("<h2>Televizyon (Samsung)</h2><p>IP: ") + htmlKacir(nvsOku("tvip")) + "</p><form action=/ayar method=post><input name=tvip placeholder='192.168.1.50' style='width:100%'><button>Kaydet</button></form>";
  h += String("<h2>Ev cihazlari</h2><div class=k>") + htmlKacir(cihazAdlari()) + "</div><form action=/ayar method=post><input name=cad placeholder='ad (tek kelime)' style='width:100%'><input name=con placeholder='ac adresi http://...' style='width:100%'><input name=coff placeholder='kapat adresi http://...' style='width:100%'><button>Ekle</button></form><form action=/sil><input type=hidden name=ne value=cihaz><button>Cihazlari sil</button></form>";
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
  if (webSunucu.hasArg("tvip")) { String t = webSunucu.arg("tvip"); t.trim(); if (t.length() >= 7 && t.length() < 40) nvsYaz("tvip", t); }
  if (webSunucu.hasArg("cad")) cihazEkle(webSunucu.arg("cad"), webSunucu.arg("con"), webSunucu.arg("coff"));
  bool yb = false;
  if (webSunucu.hasArg("wifiad")) { String a = webSunucu.arg("wifiad"); a.trim(); if (a.length() > 0) { nvsYaz("wifi", a); nvsYaz("sifre", webSunucu.arg("wifisifre")); yb = true; } }
  webSunucu.sendHeader("Location", "/");
  webSunucu.send(303);
  if (yb) { delay(1500); ESP.restart(); }
}

void webSil() {
  String ne = webSunucu.arg("ne");
  if (ne == "hafiza") nvsYaz("hafiza", "");
  else if (ne == "not") notTemizle();
  else if (ne == "liste") { nvsYaz("l_alis", ""); nvsYaz("l_yap", ""); }
  else if (ne == "alarm") alarmTemizle();
  else if (ne == "cihaz") nvsYaz("cihaz", "");
  webSunucu.sendHeader("Location", "/");
  webSunucu.send(303);
}

void webGorevi(void* p) {
  while (true) {
    webSunucu.handleClient();
    vTaskDelay(8 / portTICK_PERIOD_MS);
  }
}

void webTv() {
  webSunucu.sendHeader("Access-Control-Allow-Origin", "*");
  String url = webSunucu.arg("url"), plat = webSunucu.arg("platform"), film = webSunucu.arg("film");
  webSunucu.send(200, "text/plain", "tamam");
  if (nvsOku("tvip").length() < 7 || (url.length() == 0 && film.length() == 0)) return;
  tvUrlAc(plat, url, film);
}

void webBaslat() {
  if (WiFi.status() != WL_CONNECTED) return;
  String sv = nvsOku("ses");
  if (sv.length() > 0) { int v = sv.toInt(); if (v >= 10 && v <= 90) gSesAyar = v; }
  MDNS.begin("finduk");
  webSunucu.on("/", webAna);
  webSunucu.on("/ayar", webAyar);
  webSunucu.on("/sil", webSil);
  webSunucu.on("/tv", webTv);
  webSunucu.begin();
  xTaskCreate(webGorevi, "web", 8192, NULL, 1, NULL);
  gWebVar = true;
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

// ===== FIYAT ALARMI =====
// [fiyat:dolar|>|45]  kayitta "dolar > 45" olur. Esik asilinca telefona bildirim, sonra silinir
bool kurlariAl(float& usd, float& eur, float& altin) {
  String r = httpsGetGovde("open.er-api.com", "/v6/latest/USD", 6000);
  String t = sayiBul(r, "TRY");
  String e = sayiBul(r, "EUR");
  if (t.length() == 0) return false;
  usd = t.toFloat();
  eur = (e.length() > 0 && e.toFloat() > 0) ? usd / e.toFloat() : 0;
  altin = 0;
  String g = httpsGetGovde("api.gold-api.com", "/price/XAU", 6000);
  String gp = sayiBul(g, "price");
  if (gp.length() > 0) altin = gp.toFloat() / 31.1035f * usd;
  return usd > 0;
}

void fiyatKontrol() {
  String liste = nvsOku("fiyat");
  if (liste.length() == 0 || WiFi.status() != WL_CONNECTED) return;
  if (gSonFiyat != 0 && millis() - gSonFiyat < 1800000UL) return;   // 30 dk'da bir
  gSonFiyat = millis();
  float usd, eur, altin;
  if (!kurlariAl(usd, eur, altin)) return;
  int bas = 0;
  while (bas < (int)liste.length()) {
    int son = liste.indexOf('\n', bas);
    if (son < 0) son = liste.length();
    String satir = liste.substring(bas, son);
    bas = son + 1;
    satir.trim();
    int a = satir.indexOf(' ');
    if (a < 0) continue;
    int b = satir.indexOf(' ', a + 1);
    if (b < 0) continue;
    String ne = satir.substring(0, a);
    String op = satir.substring(a + 1, b);
    float esik = satir.substring(b + 1).toFloat();
    if (esik <= 0) continue;
    float v = 0;
    if (ne.indexOf("dolar") >= 0) v = usd;
    else if (ne.indexOf("euro") >= 0) v = eur;
    else if (ne.indexOf("altin") >= 0) v = altin;
    if (v <= 0) continue;
    bool oldu = (op == "<") ? (v <= esik) : (v >= esik);
    if (oldu) {
      bildir(String("Fiyat alarmi: ") + ne + " su an " + String(v, 2) + " lira (esik " + String(esik, 2) + ")");
      satirSil("fiyat", satir);
    }
  }
}

// ===== BUYUK PAKET 26: TELEFONDAN YAZ, ALISKANLIK, HIKAYE, TELEFONUMU BUL, GUN OZETI =====
// Baslikli / oncelikli bildirim (oncelik 3 normal, 5 acil)
void bildirBaslik(const String& baslik, const String& mesaj, int oncelik, const char* etiket) {
  if (WiFi.status() != WL_CONNECTED) return;
  WiFiClientSecure c;
  c.setInsecure();
  HTTPClient http;
  http.setTimeout(8000);
  if (!http.begin(c, String("https://ntfy.sh/") + SINEMA_KONU + "-robot")) return;
  http.addHeader("Content-Type", "text/plain; charset=utf-8");
  http.addHeader("Title", baslik);
  http.addHeader("Priority", String(oncelik));
  if (etiket && etiket[0]) http.addHeader("Tags", etiket);
  int kod = http.POST((uint8_t*)mesaj.c_str(), mesaj.length());
  Serial.println(String("Bildirim(") + baslik + ") -> HTTP " + kod);
  http.end();
}

String bugunTarih() {
  long t = simdiEpoch();
  if (t == 0) return "";
  time_t tt = (time_t)t;
  struct tm z;
  localtime_r(&tt, &z);
  char b[12];
  snprintf(b, sizeof(b), "%04d-%02d-%02d", z.tm_year + 1900, z.tm_mon + 1, z.tm_mday);
  return String(b);
}

// --- Telefonumu bul ---
void telefonuBul() {
  bildirBaslik("Telefonun burada!", "Finduk seni ariyor. Telefonun bu sesi cikariyor.", 5, "rotating_light");
  delay(1500);
  bildirBaslik("Telefonun burada!", "Hala buradayim!", 5, "rotating_light");
}

// --- Aliskanlik takibi ---
// [aliskanlik:su icmek] tanimlar, [yaptim:su icmek] bugune isaret koyar, [aliskanliksil:ad]
void aliskanlikYaptim(const String& ad) {
  String g = bugunTarih();
  if (g.length() == 0) return;
  satirEkle("alskn", g + " " + ad, 90);
}

String aliskanlikOzet() {
  String tan = satirlariAl("alsk_t");
  String kay = nvsOku("alskn");
  return String("Bugun ") + bugunTarih() + ". Tanimli aliskanliklar: " + (tan.length() ? tan : String("(yok)")) +
         ". Isaretlenen gunler (tarih ve ad): " + (kay.length() ? kay : String("(yok)")) + ". ";
}

String bugunYapilmayanlar() {
  String tan = nvsOku("alsk_t");
  String kay = nvsOku("alskn");
  String g = bugunTarih();
  String o;
  int bas = 0;
  while (bas < (int)tan.length()) {
    int son = tan.indexOf('\n', bas);
    if (son < 0) son = tan.length();
    String a = tan.substring(bas, son);
    bas = son + 1;
    a.trim();
    if (a.length() == 0) continue;
    if (kay.indexOf(g + " " + a) < 0) o += a + ", ";
  }
  return o;
}

bool aliskanlikMi(const String& sd) {
  return sd.indexOf("aliskanlik") >= 0 || sd.indexOf("seri") >= 0 || sd.indexOf("yaptim") >= 0 || sd.indexOf("ictim") >= 0 ||
         sd.indexOf("yuruyus") >= 0 || sd.indexOf("okudum") >= 0 || sd.indexOf("bu hafta") >= 0;
}

// --- Aile hikaye arsivi ---
void hikayeBitir() {
  if (gHikayeBaslik.length() == 0) return;
  if (gHikayeMetin.length() > 0) {
    bildirBaslik(String("Hikaye: ") + gHikayeBaslik, gHikayeMetin, 3, "book");
    String g = bugunTarih();
    satirEkle("hikaye", g + " " + gHikayeBaslik, 40);
  }
  gHikayeBaslik = "";
  gHikayeMetin = "";
}

// --- Aksam gun ozeti (21:30) ---
void gunOzetiKontrol() {
  long t = simdiEpoch();
  if (t == 0) return;
  if (nvsOku("ozet_kapali") == "1") return;
  time_t tt = (time_t)t;
  struct tm z;
  localtime_r(&tt, &z);
  if (!(z.tm_hour == 21 && z.tm_min >= 30) && z.tm_hour != 22) return;
  String g = bugunTarih();
  if (nvsOku("ozet_gun") == g) return;
  nvsYaz("ozet_gun", g);
  String o;
  String pl = planAl();
  if (pl.length()) o += String("Plan: ") + pl + "\n";
  String nt = notlariAl();
  if (nt.length()) o += String("Notlar: ") + nt + "\n";
  String al = satirlariAl("l_alis");
  if (al.length()) o += String("Alisveris: ") + al + "\n";
  String yp = satirlariAl("l_yap");
  if (yp.length()) o += String("Yapilacaklar: ") + yp + "\n";
  String ar = alarmListesi();
  if (ar.length()) o += String("Alarmlar: ") + ar + "\n";
  String yk = bugunYapilmayanlar();
  if (yk.length()) o += String("Bugun isaretlenmeyen aliskanliklar: ") + yk + "\n";
  if (o.length() == 0) o = "Bugun icin kayitli bir sey yok. Iyi aksamlar!";
  bildirBaslik("Finduk gun ozeti", o, 3, "memo");
}

// --- Telefondan Finduk'a yazma (ntfy ...-yaz kanali) ---
// ===== KUMANDA UYGULAMASI: YEREL KOMUTLAR BASLA =====
void durumGonder() {
  String m = String("surum|") + FW_SURUM + "\n";
  m += String("uyku|") + (gUyku ? "1" : "0") + "\n";
  m += String("ses|") + (int)gSesAyar + "\n";
  m += String("misafir|") + (gMisafir ? "1" : "0") + "\n";
  m += String("karakter|") + nvsOku("karakter") + "\n";
  m += String("dil|") + nvsOku("dil_modu") + "\n";
  m += String("macera|") + nvsOku("macera_ac") + "\n";
  m += String("alarm|") + alarmListesi() + "\n";
  m += String("not|") + notlariAl() + "\n";
  m += String("alis|") + satirlariAl("l_alis") + "\n";
  m += String("yap|") + satirlariAl("l_yap") + "\n";
  m += String("plan|") + satirlariAl("plan") + "\n";
  m += String("hatirlat|") + satirlariAl("hatirlat") + "\n";
  m += String("ozelgun|") + satirlariAl("ozelgun") + "\n";
  m += String("geri|") + satirlariAl("geri") + "\n";
  m += String("arac|") + satirlariAl("arac") + "\n";
  m += String("vardiya|") + vardiyaOzet() + "\n";
  m += String("yemek|") + satirlariAl("yemekplan") + "\n";
  m += String("espri|") + satirlariAl("espri") + "\n";
  m += String("cihaz|") + cihazAdlari() + "\n";
  m += String("tv|") + (nvsOku("tvip").length() ? "1" : "0") + "\n";
  String g = satirlariAl("rgunluk");
  if (g.length() > 600) g = g.substring(g.length() - 600);
  m += String("gunluk|") + g + "\n";
  m.replace("\r", "");
  if (m.length() > 3800) m = m.substring(0, 3800);
  bildirBaslik("Finduk durum", m, 1, "gear");
}

bool yerelKomut(const String& m) {
  if (!m.startsWith("#")) return false;
  String k = m.substring(1);
  k.trim();
  Serial.println(String("KUMANDA: ") + k);
  if (k == "durum") { }
  else if (k.startsWith("ses:")) { int v = k.substring(4).toInt(); if (v < 10) v = 10; if (v > 90) v = 90; gSesAyar = v; nvsYaz("ses", String(v)); }
  else if (k == "uyu") gUyku = true;
  else if (k == "uyan") { gUyku = false; gMood = M_MUTLU; gMoodBitis = millis() + 3000; }
  else if (k.startsWith("soyle:")) { String t = k.substring(6); t.trim(); if (t.length()) { gUyku = false; bip(1200, 120); konus(t); gKonustu = true; } }
  else if (k.startsWith("komut:")) komutIsle(k.substring(6));
  else return false;
  durumGonder();
  return true;
}
// ===== KUMANDA UYGULAMASI BITIS =====

String telefonMesajlariAl() {
  if (WiFi.status() != WL_CONNECTED) return "";
  if (gSonYazKontrol != 0 && millis() - gSonYazKontrol < 50000UL) return "";
  gSonYazKontrol = millis();
  String yol = String("/") + SINEMA_KONU + "-yaz/json?poll=1&since=" + (gYazSon.length() ? gYazSon : String("2m"));
  String r = httpsGetGovde("ntfy.sh", yol.c_str(), 4000);
  String hepsi;
  int bas = 0;
  while (bas < (int)r.length()) {
    int son = r.indexOf('\n', bas);
    if (son < 0) son = r.length();
    String satir = r.substring(bas, son);
    bas = son + 1;
    if (satir.indexOf("\"event\":\"message\"") < 0) continue;
    String id = degerBul(satir, "id");
    String m = degerBul(satir, "message");
    if (id.length()) gYazSon = id;
    if (yerelKomut(m)) continue;
    if (m.length()) hepsi += (hepsi.length() ? String(" / ") : String("")) + m;
  }
  return hepsi;
}

void telefonMesajiIsle() {
  String m = telefonMesajlariAl();
  if (m.length() == 0) return;
  Serial.println(String("TELEFONDAN: ") + m);
  String ham = claudeCevapla(m, String("Bu mesaj kullanicinin telefonundan YAZILI geldi, sesli degil. Cevabin telefona yazili gidecek; kisa yaz. Saat ve tarih: ") + zamanYazi() + ". " + aliskanlikOzet());
  String cevap = degerBul(ham, "text");
  if (cevap.length() == 0) { bildir("Mesajini aldim ama su an cevap uretemedim."); return; }
  String hamCevap = cevap;
  String komutlar[6];
  int n = komutlariAyikla(cevap, komutlar, 6);
  moodCoz(cevap);
  hafizayaEkle(m, hamCevap);
  bildirBaslik("Finduk", cevap, 3, "speech_balloon");
  for (int i = 0; i < n; i++) { komutIsle(komutlar[i]); delay(200); }
}

// ===== BUYUK PAKET 27: OZEL GUNLER, TEKRARLAYAN HATIRLATICI, GUZEL ANLAR GUNLUGU, EMANET MESAJ =====
String tarihKaydir(int gunGeri) {
  long t = simdiEpoch();
  if (t == 0) return "";
  time_t tt = (time_t)(t - (long)gunGeri * 86400L);
  struct tm z;
  localtime_r(&tt, &z);
  char b[12];
  snprintf(b, sizeof(b), "%04d-%02d-%02d", z.tm_year + 1900, z.tm_mon + 1, z.tm_mday);
  return String(b);
}

// --- Ozel gunler: satir "AA-GG ad" ([ozelgun:02-16|evlilik yildonumu]) ---
int ozelGunKalan(const String& aaGG) {
  long t = simdiEpoch();
  if (t == 0 || aaGG.length() < 5) return -1;
  time_t tt = (time_t)t;
  struct tm z;
  localtime_r(&tt, &z);
  int ay = aaGG.substring(0, 2).toInt();
  int gun = aaGG.substring(3, 5).toInt();
  if (ay < 1 || ay > 12 || gun < 1 || gun > 31) return -1;
  struct tm h = {};
  h.tm_year = z.tm_year; h.tm_mon = ay - 1; h.tm_mday = gun; h.tm_hour = 12;
  struct tm b = {};
  b.tm_year = z.tm_year; b.tm_mon = z.tm_mon; b.tm_mday = z.tm_mday; b.tm_hour = 12;
  time_t th = mktime(&h), tb = mktime(&b);
  if (th < tb) { h.tm_year += 1; th = mktime(&h); }
  return (int)((th - tb + 3600) / 86400);
}

String ozelGunler(int enFazlaGun) {
  String liste = nvsOku("ozelgun");
  String o;
  int bas = 0;
  while (bas < (int)liste.length()) {
    int son = liste.indexOf('\n', bas);
    if (son < 0) son = liste.length();
    String satir = liste.substring(bas, son);
    bas = son + 1;
    satir.trim();
    if (satir.length() < 7) continue;
    int k = ozelGunKalan(satir.substring(0, 5));
    if (k < 0 || k > enFazlaGun) continue;
    String ad = satir.substring(6);
    if (k == 0) o += String("BUGUN ") + ad + ". ";
    else o += ad + ": " + k + " gun kaldi. ";
  }
  return o;
}

void ozelGunKontrol() {
  long t = simdiEpoch();
  if (t == 0) return;
  time_t tt = (time_t)t;
  struct tm z;
  localtime_r(&tt, &z);
  if (z.tm_hour < 9) return;
  String g = bugunTarih();
  if (nvsOku("og_gun") == g) return;
  nvsYaz("og_gun", g);
  String liste = nvsOku("ozelgun");
  int bas = 0;
  while (bas < (int)liste.length()) {
    int son = liste.indexOf('\n', bas);
    if (son < 0) son = liste.length();
    String satir = liste.substring(bas, son);
    bas = son + 1;
    satir.trim();
    if (satir.length() < 7) continue;
    int k = ozelGunKalan(satir.substring(0, 5));
    String ad = satir.substring(6);
    if (k == 0) bildirBaslik("Bugun ozel bir gun!", ad, 4, "tada");
    else if (k == 1) bildirBaslik("Yarin ozel gun", ad + " yarin. Hazirlik yaptin mi?", 4, "gift");
    else if (k == 7) bildirBaslik("Bir hafta kaldi", ad + " icin 7 gun kaldi.", 3, "calendar");
  }
}

// --- Tekrarlayan hatirlatici: satir "SS:DD gunler mesaj" ([hatirlat:19:00|pzt,cars|cop cikacak]) ---
bool gunUyar(const String& gunler, int wday) {
  if (gunler.indexOf("hergun") >= 0) return true;
  if (gunler.indexOf("haftaici") >= 0) return wday >= 1 && wday <= 5;
  if (gunler.indexOf("haftasonu") >= 0) return wday == 0 || wday == 6;
  const char* k[] = {"paz", "pzt", "sal", "car", "per", "cum", "cmt"};
  return gunler.indexOf(k[wday]) >= 0;
}

void hatirlaticiKontrol() {
  long now = simdiEpoch();
  if (now == 0) return;
  if (gSonHatirlat == 0 || now - gSonHatirlat > 6L * 3600L) { gSonHatirlat = now; return; }
  String liste = nvsOku("hatirlat");
  if (liste.length() == 0) { gSonHatirlat = now; return; }
  time_t tt = (time_t)now;
  struct tm z;
  localtime_r(&tt, &z);
  int bas = 0;
  while (bas < (int)liste.length()) {
    int son = liste.indexOf('\n', bas);
    if (son < 0) son = liste.length();
    String satir = liste.substring(bas, son);
    bas = son + 1;
    satir.trim();
    if (satir.length() < 8 || satir[2] != ':') continue;
    int sa = satir.substring(0, 2).toInt();
    int dk = satir.substring(3, 5).toInt();
    int b2 = satir.indexOf(' ', 6);
    if (b2 < 0) continue;
    String gunler = satir.substring(6, b2);
    String mesaj = satir.substring(b2 + 1);
    if (!gunUyar(gunler, z.tm_wday)) continue;
    struct tm h = z;
    h.tm_hour = sa; h.tm_min = dk; h.tm_sec = 0;
    long hedef = (long)mktime(&h);
    if (hedef > gSonHatirlat && hedef <= now) {
      bildirBaslik("Hatirlatma", mesaj, 4, "bell");
      if (z.tm_hour >= 8 && z.tm_hour < 23) { bip(1200, 150); konus(String("Hatirlatma: ") + mesaj); }
    }
  }
  gSonHatirlat = now;
}

// --- Guzel anlar gunlugu: satir "YYYY-AA-GG metin" ---
String haftaninAnlari() {
  String liste = nvsOku("gunluk");
  String sinir = tarihKaydir(6);
  String o;
  int bas = 0;
  while (bas < (int)liste.length()) {
    int son = liste.indexOf('\n', bas);
    if (son < 0) son = liste.length();
    String satir = liste.substring(bas, son);
    bas = son + 1;
    satir.trim();
    if (satir.length() < 12) continue;
    if (satir.substring(0, 10) >= sinir) o += String("- ") + satir.substring(11) + "\n";
  }
  return o;
}

void haftalikAnlarKontrol() {
  long t = simdiEpoch();
  if (t == 0) return;
  time_t tt = (time_t)t;
  struct tm z;
  localtime_r(&tt, &z);
  if (z.tm_wday != 0 || z.tm_hour < 20) return;
  String g = bugunTarih();
  if (nvsOku("anlar_gun") == g) return;
  nvsYaz("anlar_gun", g);
  String a = haftaninAnlari();
  if (a.length() > 0) bildirBaslik("Bu haftanin guzel anlari", a, 3, "sparkles");
}

// --- Emanet mesaj: satir "kime: mesaj" ([emanet:Demet|aksam gec gelecegim]) ---
String emanetler() { return nvsOku("emanet"); }

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
  if (gSesIlk) gSesYeniBasladi = true;
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
  // Kucuk hoparlor cizirtisina karsi: derin basi kes (yuksek geciren ~180 Hz), %75 pay birak, tepeleri yumusak sinirla
  static float hpX = 0, hpY = 0;
  if (gSesYeniBasladi) { hpX = 0; hpY = 0; gSesYeniBasladi = false; }
  const float hpA = 0.94f;
  int vol = MAC_SES ? 0 : sesYuzdeSimdi();
  for (int i = 0; i < yaz_n / 2; i++) {
    float x = (float)s[i];
    hpY = hpA * (hpY + x - hpX);
    hpX = x;
    float v = hpY * (float)vol * 0.0055f;
    float a = fabsf(v);
    const float diz = 11000.0f;
    if (a > diz) { a = diz + (a - diz) / (1.0f + (a - diz) / 6000.0f); v = (v < 0) ? -a : a; }
    if (v > 30000.0f) v = 30000.0f;
    if (v < -30000.0f) v = -30000.0f;
    s[i] = (int16_t)v;
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

// ===== BUYUK PAKET 28: KARAKTER VE YASAM PAKETI BASLA =====
String rastgeleSatirlar(const char* anahtar, int n) {
  String s = nvsOku(anahtar);
  String satir[40];
  int say = 0, bas = 0;
  while (bas < (int)s.length() && say < 40) {
    int e = s.indexOf('\n', bas);
    if (e < 0) e = s.length();
    String t = s.substring(bas, e);
    bas = e + 1;
    t.trim();
    if (t.length()) satir[say++] = t;
  }
  String o;
  for (int i = 0; i < n && say > 0; i++) {
    int k = esp_random() % say;
    o += satir[k] + "; ";
    satir[k] = satir[say - 1];
    say--;
  }
  return o;
}

long gunFarki(const String& t10) {
  long now = simdiEpoch();
  if (now == 0 || t10.length() < 10) return -9999;
  struct tm t = {};
  t.tm_year = t10.substring(0, 4).toInt() - 1900;
  t.tm_mon = t10.substring(5, 7).toInt() - 1;
  t.tm_mday = t10.substring(8, 10).toInt();
  t.tm_hour = 12;
  t.tm_isdst = -1;
  time_t h = mktime(&t);
  time_t tt = (time_t)now;
  struct tm z;
  localtime_r(&tt, &z);
  z.tm_hour = 12; z.tm_min = 0; z.tm_sec = 0; z.tm_isdst = -1;
  time_t b = mktime(&z);
  return (long)lround((double)(h - b) / 86400.0);
}

// --- Gunluk sayaclar (robotun gunlugu icin) ---
void sayacTazele() {
  String g = bugunTarih();
  if (g.length() && nvsOku("say_gun") != g) {
    nvsYaz("say_gun", g);
    nvsYaz("say_k", "0");
    nvsYaz("say_kendi", "0");
    nvsYaz("say_su", "0");
  }
}
int sayacAl(const char* k) { sayacTazele(); return nvsOku(k).toInt(); }
void sayacArttir(const char* k) { sayacTazele(); nvsYaz(k, String(nvsOku(k).toInt() + 1)); }

// --- Huy: gunun havasi + uzun suredir gorusmeme ---
String huyBilgisi() {
  long now = simdiEpoch();
  if (now == 0) return "";
  const char* huy[] = {"neseli ve enerjik", "sakin ve dusunceli", "merakli, bol soru soran", "biraz tembel ve uykulu", "sakaci", "hafif nostaljik"};
  time_t tt = (time_t)now;
  struct tm z;
  localtime_r(&tt, &z);
  int h = (z.tm_yday * 7 + z.tm_year) % 6;
  String s = String(" Bugunku huyun: ") + huy[h] + " (abartma, cevaplara hafifce yansit).";
  long son = nvsOku("son_gorus").toInt();
  if (son > 0) {
    long saat = (now - son) / 3600;
    if (saat >= 72) s += String(" Kullaniciyi ") + (saat / 24) + " gundur gormedin, onu ozledin; bu konusmada ozledigini bir cumleyle sicak ve sirin sekilde belirt.";
    else if (saat >= 44) s += " Kullanici iki gundur seninle konusmadi, biraz kuskunsun ama sevimli bir sekilde (tek cumle, agir kirginlik yok).";
  }
  return s;
}

// claudeCevapla sistem metnine eklenen paket 28 parcalari
String paket28Sistem() {
  String s = "";
  if (gMisafir) {
    s += " MISAFIR MODU ACIK: Evde misafir var. Sevimli ve esprili ol, kendini tanitabilirsin. Oguzhan'in ya da ailesinin kisisel bilgilerini (notlar, planlar, hafiza, aile, is) ASLA soyleme, sorulursa bunun ozel oldugunu soyle.";
    return s;
  }
  if (gKisi == "demet") s += " Su an konusan kisi Demet (Oguzhan'in esi), Oguzhan degil. Ona adiyla hitap et.";
  s += huyBilgisi();
  String es = rastgeleSatirlar("espri", 3);
  if (es.length()) s += String(" Ikinizin ortak espri ve anilari (ara sira, dogal bir yerde, zorlamadan an): ") + es;
  if (nvsOku("macera_ac") == "1") s += String(" SESLI MACERA OYUNU ACIK. Tema ve hikaye ozeti: ") + nvsOku("macera") + ". Anlatici gibi 3-4 kisa cumleyle surdur, sonda kullaniciya ne yapacagini sor. Her cevabinin sonuna [macera:ozet|hikayenin guncel ozeti, en fazla 300 karakter] yaz. Oyun bitince [macera:bitir] yaz. Bu modda 2 cumle siniri yoktur.";
  String kr = nvsOku("karakter");
  if (kr.length()) s += String(" KARAKTER MODU ACIK: Su karakteri oynuyorsun: ") + kr + ". Cevaplarini bu agizla ver, kisa kal, Finduk oldugunu inkar etme.";
  String dl = nvsOku("dil_modu");
  if (dl.length()) s += String(" INGILIZCE PRATIK MODU ACIK (seviye: ") + dl + "). Kullanici Ingilizce pratik yapiyor. Cevabini basit Ingilizce ver (1-2 kisa cumle) ve bir soruyla devam et. Kullanici hata yaptiysa cevabin sonunda Kucuk duzeltme diyerek Turkce kisaca duzelt. Kullanici Turkce konusursa Turkce yardim et.";
  s += paket29Sistem();
  return s;
}

// --- Bir yil once bugun: anilar kutusu ---
String yilOnceAni() {
  long now = simdiEpoch();
  if (now == 0) return "";
  time_t tt = (time_t)now;
  struct tm z;
  localtime_r(&tt, &z);
  char k[8];
  snprintf(k, sizeof(k), "an%02d%02d", z.tm_mon + 1, z.tm_mday);
  String s = nvsOku(k);
  String o;
  int yil = z.tm_year + 1900;
  int bas = 0;
  while (bas < (int)s.length()) {
    int e = s.indexOf('\n', bas);
    if (e < 0) e = s.length();
    String satir = s.substring(bas, e);
    bas = e + 1;
    satir.trim();
    int y = satir.substring(0, 4).toInt();
    if (satir.length() > 5 && y > 2000 && y < yil) o += satir + "; ";
  }
  return o;
}

void aniKaydet(const String& metin) {
  long now = simdiEpoch();
  if (now == 0) return;
  time_t tt = (time_t)now;
  struct tm z;
  localtime_r(&tt, &z);
  char k[8];
  snprintf(k, sizeof(k), "an%02d%02d", z.tm_mon + 1, z.tm_mday);
  satirEkle(k, String(z.tm_year + 1900) + " " + metin, 4);
}

// --- Robotun kendiliginden konusmasi (kimse sormadan) ---
bool robotKonusur(const String& soru, const String& bilgi) {
  if (WiFi.status() != WL_CONNECTED) return false;
  gDurum = D_DUSUN;
  String ham = claudeCevapla(soru, bilgi);
  String cevap = degerBul(ham, "text");
  if (cevap.length() == 0) { gDurum = D_BOSTA; return false; }
  String tut[4];
  int n = komutlariAyikla(cevap, tut, 4);
  int mood = moodCoz(cevap);
  Serial.print("FINDUK (kendiliginden): ");
  Serial.println(cevap);
  gUyku = false;
  gMood = mood;
  konus(cevap);
  for (int i = 0; i < n; i++) {
    if (gMisafir && !tut[i].startsWith("goz") && !tut[i].startsWith("misafir")) continue;
    komutIsle(tut[i]);
  }
  gMoodBitis = millis() + 2500;
  sohbetBitis = millis() + SOHBET_SURE;
  gKonustu = true;
  gDurum = D_BOSTA;
  return true;
}

void kendiKonus() {
  long now = simdiEpoch();
  if (now == 0 || gMisafir || WiFi.status() != WL_CONNECTED) return;
  time_t tt = (time_t)now;
  struct tm z;
  localtime_r(&tt, &z);
  if (z.tm_hour < 9 || z.tm_hour >= 22) return;
  if (millis() - gSonEtkilesim < 45UL * 60000UL) return;
  if (gSonKendi != 0 && millis() - gSonKendi < 60UL * 60000UL) return;
  if (sayacAl("say_kendi") >= 4) return;
  if (esp_random() % 2 != 0) return;
  gSonKendi = millis();
  sayacArttir("say_kendi");
  String bilgi = "KENDILIGINDEN KONUSMA: Kimse sana bir sey sormadi, sessizlik oldu. Kisa (en fazla 2 cumle), dogal ve sicak bir sey soyle. Su seceneklerden birini sec: gunun nasil gectigini sor; ortak bir espriyi an; ilginc kucuk bir bilgi ver; tarihte bugun olan guzel bir olay anlat; ya da asagidaki ipuclarindan birini kullan. Kendini tanitma. Emin olmadigin bilgiyi uydurma.";
  String an = yilOnceAni();
  if (an.length()) bilgi += String(" Bir yil once bugun yasanan anilar: ") + an;
  String og = ozelGunler(30);
  if (og.length()) bilgi += String(" Yaklasan ozel gunler: ") + og;
  robotKonusur("(Sessizlik oldu, kendiliginden bir sey soyle)", bilgi);
}

// --- Robotun kendi gunlugu (aksam 21:45'ten sonra) ---
void robotGunlukKontrol() {
  long now = simdiEpoch();
  if (now == 0 || WiFi.status() != WL_CONNECTED) return;
  time_t tt = (time_t)now;
  struct tm z;
  localtime_r(&tt, &z);
  if (z.tm_hour < 21 || (z.tm_hour == 21 && z.tm_min < 45)) return;
  String g = bugunTarih();
  if (nvsOku("rg_gun") == g) return;
  nvsYaz("rg_gun", g);
  int k = sayacAl("say_k"), kd = sayacAl("say_kendi"), su = sayacAl("say_su");
  if (k == 0 && kd == 0) return;
  String anlar = haftaninAnlari();
  if (anlar.length() > 400) anlar = anlar.substring(0, 400);
  String bilgi = String("ROBOT GUNLUGU: Bugun icin kendi gunlugunu yaz. Birinci agizdan (ben), en fazla 2 kisa cumle (toplam 120 karakteri gecmesin), sicak ve biraz esprili. Sayilar: kullanici ile ") + k + " konusma, kendiliginden " + kd + " konusma, su hatirlatmasi " + su + ". Bu haftanin guzel anlari: " + anlar + " Ortak espriler: " + rastgeleSatirlar("espri", 2) + " Sadece bu bilgilere dayan, uydurma. Cevabin sonuna komut yazma.";
  String ham = claudeCevapla("(Gunlugunu yaz)", bilgi);
  String c = degerBul(ham, "text");
  if (c.length() == 0) return;
  String tut[2];
  komutlariAyikla(c, tut, 2);
  moodCoz(c);
  satirEkle("rgunluk", g + " " + c, 14);
  bildirBaslik("Finduk'un gunlugu", c, 3, "book");
}

void yilOnceKontrol() {
  long now = simdiEpoch();
  if (now == 0) return;
  time_t tt = (time_t)now;
  struct tm z;
  localtime_r(&tt, &z);
  if (z.tm_hour < 9) return;
  String g = bugunTarih();
  if (nvsOku("yo_gun") == g) return;
  nvsYaz("yo_gun", g);
  String an = yilOnceAni();
  if (an.length()) bildirBaslik("Bir yil once bugun", an, 3, "hourglass");
}

// --- Bayram ve ozel gun modlari ---
bool hicriAyGun(int& ay, int& gun) {
  long t = simdiEpoch();
  if (t == 0) return false;
  time_t tt = (time_t)t;
  struct tm z;
  localtime_r(&tt, &z);
  char yol[64];
  snprintf(yol, sizeof(yol), "/v1/gToH/%02d-%02d-%04d", z.tm_mday, z.tm_mon + 1, z.tm_year + 1900);
  String r = httpsGetGovde("api.aladhan.com", yol, 4000);
  int k = r.indexOf("\"hijri\"");
  if (k < 0) return false;
  String seg = r.substring(k);
  int g = seg.indexOf("\"gregorian\"");
  if (g > 0) seg = seg.substring(0, g);
  String gs = degerBul(seg, "day");
  String as = sayiBul(seg, "number");
  if (gs.length() == 0 || as.length() == 0) return false;
  gun = gs.toInt();
  ay = as.toInt();
  return gun > 0 && ay > 0;
}

void ozelModKontrol() {
  long now = simdiEpoch();
  if (now == 0 || WiFi.status() != WL_CONNECTED) return;
  time_t tt = (time_t)now;
  struct tm z;
  localtime_r(&tt, &z);
  if (z.tm_hour < 8 || z.tm_hour >= 22) return;
  String g = bugunTarih();
  if (nvsOku("om_gun") == g) return;
  nvsYaz("om_gun", g);
  char k[8];
  snprintf(k, sizeof(k), "%02d-%02d", z.tm_mon + 1, z.tm_mday);
  const char* sabit[][2] = {
    {"01-01", "Yilbasi"}, {"03-08", "Dunya Kadinlar Gunu"}, {"03-18", "Canakkale Zaferi anma gunu"},
    {"04-23", "23 Nisan Ulusal Egemenlik ve Cocuk Bayrami"}, {"05-01", "Emek ve Dayanisma Gunu"},
    {"05-19", "19 Mayis Ataturk'u Anma, Genclik ve Spor Bayrami"}, {"07-15", "15 Temmuz Demokrasi ve Milli Birlik Gunu"},
    {"08-30", "30 Agustos Zafer Bayrami"}, {"10-29", "29 Ekim Cumhuriyet Bayrami"},
    {"11-10", "10 Kasim Ataturk'u anma gunu (saygiyla, sakin bir dille)"}, {"11-24", "Ogretmenler Gunu"}
  };
  String ad = "";
  for (unsigned int i = 0; i < sizeof(sabit) / sizeof(sabit[0]); i++) if (String(sabit[i][0]) == k) ad = sabit[i][1];
  if (ad.length() == 0) {
    int ay = 0, gun = 0;
    if (hicriAyGun(ay, gun)) {
      if (ay == 9 && gun == 1) ad = "Ramazan ayinin ilk gunu";
      else if (ay == 10 && gun >= 1 && gun <= 3) ad = "Ramazan Bayrami";
      else if (ay == 12 && gun == 9) ad = "Kurban Bayrami arifesi";
      else if (ay == 12 && gun >= 10 && gun <= 13) ad = "Kurban Bayrami";
      else if (ay == 1 && gun == 1) ad = "Hicri yilbasi";
      else if (ay == 1 && gun == 10) ad = "Asure gunu";
      else if (ay == 3 && gun == 11) ad = "Mevlid Kandili (bu aksam)";
      else if (ay == 7 && gun == 26) ad = "Mirac Kandili (bu aksam)";
      else if (ay == 8 && gun == 14) ad = "Berat Kandili (bu aksam)";
      else if (ay == 9 && gun == 26) ad = "Kadir Gecesi (bu aksam)";
    }
  }
  if (ad.length() == 0) return;
  gOzel = 2;
  gOzelBitis = millis() + 8000;
  bildirBaslik(ad, "Finduk bugunu kutluyor", 3, "tada");
  robotKonusur(String("(Bugun: ") + ad + ". Bu gune uygun, sicak ve kisa bir tebrik ya da anma sozu soyle)", String("Bugun ") + ad + ". Emin degilsen sadece guzel bir gun dile, tarihi bilgi uydurma.");
}

// --- Su ve mola hatirlaticisi ---
void suKur(const String& ar) {
  if (sadelestir(ar).indexOf("kapat") >= 0) { nvsYaz("su", ""); return; }
  int ar1 = ar.toInt();
  if (ar1 < 20) ar1 = 20;
  if (ar1 > 240) ar1 = 240;
  int bas = 9, bit = 22;
  int c1 = ar.indexOf('|');
  if (c1 > 0) {
    bas = ar.substring(c1 + 1).toInt();
    int c2 = ar.indexOf('|', c1 + 1);
    if (c2 > 0) bit = ar.substring(c2 + 1).toInt();
  }
  if (bas < 0 || bas > 23) bas = 9;
  if (bit < 1 || bit > 24) bit = 22;
  nvsYaz("su", String(ar1) + " " + bas + " " + bit);
  gSonSu = millis();
}

void suKontrol() {
  String c = nvsOku("su");
  if (c.length() < 5) return;
  long now = simdiEpoch();
  if (now == 0) return;
  int b1 = c.indexOf(' ');
  int b2 = c.indexOf(' ', b1 + 1);
  if (b1 < 0 || b2 < 0) return;
  int ar = c.toInt();
  int bas = c.substring(b1 + 1, b2).toInt();
  int bit = c.substring(b2 + 1).toInt();
  time_t tt = (time_t)now;
  struct tm z;
  localtime_r(&tt, &z);
  if (z.tm_hour < bas || z.tm_hour >= bit) return;
  if (gSonSu == 0) { gSonSu = millis(); return; }
  if (millis() - gSonSu < (unsigned long)ar * 60000UL) return;
  gSonSu = millis();
  const char* m[] = {"Su icme vakti! Bir bardak su iyi gelir.", "Biraz ayaga kalk, omuzlarini ve boynunu gevset.", "Gozlerini dinlendir, birkac saniye uzaga bak.", "Bir yudum su, bir derin nefes. Hadi.", "Kisa bir mola zamani, biraz yuru."};
  bip(1200, 150);
  gUyku = false;
  konus(m[esp_random() % 5]);
  sayacArttir("say_su");
  gKonustu = true;
}

// --- Arac bakim takvimi: satir "YYYY-AA-GG ad" ---
void aracKontrol() {
  long now = simdiEpoch();
  if (now == 0) return;
  time_t tt = (time_t)now;
  struct tm z;
  localtime_r(&tt, &z);
  if (z.tm_hour < 10) return;
  String g = bugunTarih();
  if (nvsOku("ar_gun") == g) return;
  nvsYaz("ar_gun", g);
  String liste = nvsOku("arac");
  int bas = 0;
  while (bas < (int)liste.length()) {
    int e = liste.indexOf('\n', bas);
    if (e < 0) e = liste.length();
    String satir = liste.substring(bas, e);
    bas = e + 1;
    satir.trim();
    if (satir.length() < 12) continue;
    long k = gunFarki(satir.substring(0, 10));
    String ad = satir.substring(11);
    if (k == 30 || k == 14 || k == 7 || k == 3 || k == 1) bildirBaslik(String("Arac: ") + ad, String(k) + " gun kaldi", 4, "car");
    else if (k == 0) bildirBaslik(String("Arac: ") + ad, "Bugun son gun!", 5, "warning");
  }
}

String aracOzet() {
  String liste = nvsOku("arac");
  if (liste.length() == 0) return "Kayitli arac takvimi yok.";
  String o = "Arac takvimi: ";
  int bas = 0;
  while (bas < (int)liste.length()) {
    int e = liste.indexOf('\n', bas);
    if (e < 0) e = liste.length();
    String satir = liste.substring(bas, e);
    bas = e + 1;
    satir.trim();
    if (satir.length() < 12) continue;
    long k = gunFarki(satir.substring(0, 10));
    o += satir.substring(11) + ": " + (k >= 0 ? String(k) + " gun kaldi" : String(-k) + " gun once") + "; ";
  }
  return o;
}

// --- Vardiya takvimi: "YYYY-AA-GG sabah,aksam,gece" (baslangic haftasi pazartesi) ---
void vardiyaKur(const String& ar) {
  if (sadelestir(ar).indexOf("kapat") >= 0) { nvsYaz("vardiya", ""); return; }
  int c = ar.indexOf('|');
  if (c != 10) return;
  String l = ar.substring(c + 1);
  l.replace(" ", "");
  l.toLowerCase();
  nvsYaz("vardiya", ar.substring(0, 10) + " " + temizMetin(l, 60));
}

String vardiyaOzet() {
  String v = nvsOku("vardiya");
  if (v.length() < 12) return "";
  long gecen = -gunFarki(v.substring(0, 10));
  String liste = v.substring(11);
  liste.trim();
  String t[8];
  int n = 0, bas = 0;
  while (bas < (int)liste.length() && n < 8) {
    int e = liste.indexOf(',', bas);
    if (e < 0) e = liste.length();
    String x = liste.substring(bas, e);
    x.trim();
    bas = e + 1;
    if (x.length()) t[n++] = x;
  }
  if (n == 0) return "";
  if (gecen < 0) return String("Vardiya duzeni henuz baslamadi, baslangic ") + v.substring(0, 10) + ".";
  long hafta = gecen / 7;
  return String("Vardiya (donen duzen): bu hafta ") + t[hafta % n] + ", gelecek hafta " + t[(hafta + 1) % n] + ", iki hafta sonra " + t[(hafta + 2) % n] + ".";
}

void vardiyaKontrol() {
  long now = simdiEpoch();
  if (now == 0) return;
  time_t tt = (time_t)now;
  struct tm z;
  localtime_r(&tt, &z);
  if (z.tm_wday != 0 || z.tm_hour < 18) return;
  String g = bugunTarih();
  if (nvsOku("vd_gun") == g) return;
  nvsYaz("vd_gun", g);
  String o = vardiyaOzet();
  if (o.length()) bildirBaslik("Haftalik vardiya", o, 3, "calendar");
}

// --- Mac gunu: satir "YYYY-AA-GG SS:DD A takim" (A: 0 bekliyor, 1 on bildirim, 2 basladi) ---
void macKur(const String& ar) {
  int c1 = ar.indexOf('|');
  if (c1 <= 0) return;
  String takim = ar.substring(0, c1);
  String rest = ar.substring(c1 + 1);
  int c2 = rest.indexOf('|');
  String saat = (c2 < 0) ? rest : rest.substring(0, c2);
  String tarih = (c2 < 0) ? bugunTarih() : rest.substring(c2 + 1);
  saat.trim();
  tarih.trim();
  if (saat.length() != 5 || saat[2] != ':' || tarih.length() != 10) return;
  satirEkle("macgun", tarih + " " + saat + " 0 " + takim, 6);
}

void macKontrol() {
  String liste = nvsOku("macgun");
  if (liste.length() == 0) return;
  long now = simdiEpoch();
  if (now == 0) return;
  String yeni = "";
  bool degisti = false;
  int bas = 0;
  while (bas < (int)liste.length()) {
    int e = liste.indexOf('\n', bas);
    if (e < 0) e = liste.length();
    String satir = liste.substring(bas, e);
    bas = e + 1;
    satir.trim();
    if (satir.length() < 20) { degisti = true; continue; }
    struct tm t = {};
    t.tm_year = satir.substring(0, 4).toInt() - 1900;
    t.tm_mon = satir.substring(5, 7).toInt() - 1;
    t.tm_mday = satir.substring(8, 10).toInt();
    t.tm_hour = satir.substring(11, 13).toInt();
    t.tm_min = satir.substring(14, 16).toInt();
    t.tm_isdst = -1;
    long kick = (long)mktime(&t);
    int as = satir.charAt(17) - '0';
    String takim = satir.substring(19);
    bool sil = false;
    if (now > kick + 6 * 3600L) {
      sil = true;
    } else if (now >= kick + 6900L) {
      sil = true;
      bildirBaslik(takim + " maci", "Mac bitmis olmali, Finduk skoru soyluyor.", 3, "soccer");
      robotKonusur(String("(") + takim + " maci bitmis olmali, skoru ve sonucu kisaca soyle)", String("Takim: ") + takim + ". " + macBilgisi() + " Skoru bulamazsan bulamadigini soyle, uydurma.");
    } else if (as < 2 && now >= kick) {
      as = 2;
      degisti = true;
      gOzel = 3;
      gOzelBitis = millis() + 25000;
      gMood = M_MUTLU;
      gMoodBitis = millis() + 5000;
      gUyku = false;
      konus(takim + " maci basladi, heyecan basliyor!");
      gKonustu = true;
    } else if (as == 0 && now >= kick - 600) {
      as = 1;
      degisti = true;
      bildirBaslik(takim + " maci", "10 dakika sonra basliyor", 4, "soccer");
      gUyku = false;
      bip(1200, 150);
      konus(takim + " maci 10 dakika sonra basliyor!");
      gKonustu = true;
    }
    if (sil) { degisti = true; continue; }
    yeni += satir.substring(0, 17) + String(as) + satir.substring(18) + "\n";
  }
  if (degisti) nvsYaz("macgun", yeni);
}

// --- Gizli kasa: satir "parola not" (parola tek kelime, kucuk harf) ---
String kasaAc(const String& sd) {
  String s = nvsOku("kasa");
  String o;
  int bas = 0;
  while (bas < (int)s.length()) {
    int e = s.indexOf('\n', bas);
    if (e < 0) e = s.length();
    String satir = s.substring(bas, e);
    bas = e + 1;
    satir.trim();
    int sp = satir.indexOf(' ');
    if (sp < 4) continue;
    if (sd.indexOf(satir.substring(0, sp)) >= 0) o += satir.substring(sp + 1) + ". ";
  }
  return o;
}

void kasaKaydet(const String& ar) {
  int c = ar.indexOf('|');
  if (c <= 0) return;
  String p = sadelestir(ar.substring(0, c));
  p.trim();
  int sp = p.indexOf(' ');
  if (sp > 0) p = p.substring(0, sp);
  if (p.length() < 4) return;
  satirEkle("kasa", p + " " + ar.substring(c + 1), 15);
}

// --- Mutfak modu: tarif adim adim ---
void tarifKur(const String& ar) {
  int c = ar.indexOf('|');
  if (c < 0) return;
  gTarifBaslik = ar.substring(0, c);
  String s = ar.substring(c + 1);
  gTarifN = 0;
  gTarifI = 0;
  int bas = 0;
  while (bas < (int)s.length() && gTarifN < 12) {
    int e = s.indexOf(';', bas);
    if (e < 0) e = s.length();
    String t = s.substring(bas, e);
    t.trim();
    bas = e + 1;
    if (t.length()) gTarifAdim[gTarifN++] = t;
  }
  if (gTarifN > 0) {
    String msg = String("Birinci adim: ") + gTarifAdim[0] + ". Hazir olunca sonraki de.";
    int sure = adimSuresi(gTarifAdim[0]);
    if (sure > 0) {
      zamanlayiciKur(String(sure) + "|" + gTarifBaslik + " icin 1. adim suresi doldu");
      msg += String(" ") + sure + " dakikalik zamanlayiciyi kurdum.";
    }
    konus(msg);
  }
}

bool tarifSozMu(const String& sd) {
  return gTarifN > 0 && (sd.indexOf("sonraki") >= 0 || sd.indexOf("tekrar") >= 0 || sd.indexOf("onceki") >= 0 || sd.indexOf("tarif bitti") >= 0 || sd.indexOf("tarifi kapat") >= 0);
}

bool tarifKomutu(const String& sd) {
  if (gTarifN <= 0) return false;
  if (sd.indexOf("tarif bitti") >= 0 || sd.indexOf("tarifi kapat") >= 0) { gTarifN = 0; konus("Afiyet olsun!"); return true; }
  if (sd.indexOf("onceki") >= 0) { if (gTarifI > 0) gTarifI--; }
  else if (sd.indexOf("sonraki") >= 0) gTarifI++;
  else if (sd.indexOf("tekrar") >= 0) { }
  else return false;
  if (gTarifI >= gTarifN) { gTarifN = 0; konus("Tarif bitti, afiyet olsun!"); return true; }
  String msg = String("Adim ") + (gTarifI + 1) + ": " + gTarifAdim[gTarifI] + ".";
  int sure = (sd.indexOf("sonraki") >= 0) ? adimSuresi(gTarifAdim[gTarifI]) : 0;
  if (sure > 0) {
    zamanlayiciKur(String(sure) + "|" + gTarifBaslik + " icin " + (gTarifI + 1) + ". adim suresi doldu");
    msg += String(" ") + sure + " dakikalik zamanlayiciyi kurdum.";
  }
  konus(msg);
  return true;
}

// --- Sesli macera ---
void maceraIsle(const String& ar) {
  int c = ar.indexOf('|');
  String islem = (c < 0) ? ar : ar.substring(0, c);
  String m = (c < 0) ? String("") : ar.substring(c + 1);
  islem.trim();
  if (islem == "basla") { nvsYaz("macera_ac", "1"); nvsYaz("macera", temizMetin(String("Tema: ") + m, 380)); }
  else if (islem == "ozet") { if (nvsOku("macera_ac") == "1") nvsYaz("macera", temizMetin(m, 380)); }
  else if (islem == "bitir") { nvsYaz("macera_ac", ""); nvsYaz("macera", ""); }
}

// --- Ev cihazlari (Shelly, Tasmota gibi yerel http ile acilip kapanan cihazlar): satir "ad ac_url kapat_url" ---
void cihazEkle(String ad, String on, String off) {
  ad = sadelestir(ad);
  ad.replace(" ", "");
  on.trim();
  off.trim();
  if (ad.length() < 2 || ad.length() > 20 || !on.startsWith("http://")) return;
  if (!off.startsWith("http://")) off = on;
  if (on.indexOf(' ') >= 0 || off.indexOf(' ') >= 0 || on.length() > 120 || off.length() > 120) return;
  satirSil("cihaz", ad + " ");
  String s = nvsOku("cihaz");
  s += ad + " " + on + " " + off + "\n";
  nvsYaz("cihaz", s);
}

String cihazAdlari() {
  String s = nvsOku("cihaz");
  String o;
  int bas = 0;
  while (bas < (int)s.length()) {
    int e = s.indexOf('\n', bas);
    if (e < 0) e = s.length();
    String satir = s.substring(bas, e);
    bas = e + 1;
    int sp = satir.indexOf(' ');
    if (sp > 0) o += satir.substring(0, sp) + ", ";
  }
  return o;
}

bool cihazKomut(const String& ar) {
  int c = ar.indexOf('|');
  if (c <= 0) return false;
  String ad = sadelestir(ar.substring(0, c));
  ad.replace(" ", "");
  String eylem = sadelestir(ar.substring(c + 1));
  bool kapat = eylem.indexOf("kapat") >= 0 || eylem.indexOf("sondur") >= 0;
  String s = nvsOku("cihaz");
  int bas = 0;
  while (bas < (int)s.length()) {
    int e = s.indexOf('\n', bas);
    if (e < 0) e = s.length();
    String satir = s.substring(bas, e);
    bas = e + 1;
    int a = satir.indexOf(' ');
    int b = satir.indexOf(' ', a + 1);
    if (a <= 0 || b < 0) continue;
    if (satir.substring(0, a) != ad) continue;
    String url = kapat ? satir.substring(b + 1) : satir.substring(a + 1, b);
    url.trim();
    HTTPClient http;
    http.setTimeout(5000);
    if (!http.begin(url)) return false;
    int kod = http.GET();
    http.end();
    Serial.println(String("CIHAZ ") + ad + (kapat ? " kapat" : " ac") + " -> HTTP " + kod);
    return kod > 0 && kod < 400;
  }
  return false;
}

// --- Samsung televizyon (WebSocket, port 8002). Ilk kullanimda TV ekraninda Izin ver denmeli ---
bool tvBekle(WiFiClientSecure& c, unsigned long ms) {
  unsigned long b = millis();
  while (!c.available() && c.connected() && millis() - b < ms) delay(10);
  return c.available() > 0;
}

int tvBayt(WiFiClientSecure& c) {
  if (!c.available()) tvBekle(c, 1500);
  return c.read();
}

// Sunucudan gelen tek bir WebSocket cercevesini okur (en fazla 24 KB saklar)
String tvCerceveOku(WiFiClientSecure& c, unsigned long ms) {
  if (!tvBekle(c, ms)) return "";
  int b0 = tvBayt(c), b1 = tvBayt(c);
  if (b0 < 0 || b1 < 0) return "";
  unsigned long len = b1 & 0x7F;
  if (len == 126) { int h = tvBayt(c), l = tvBayt(c); len = ((unsigned long)(h & 0xFF) << 8) | (l & 0xFF); }
  else if (len == 127) { len = 0; for (int i = 0; i < 8; i++) len = (len << 8) | (tvBayt(c) & 0xFF); }
  String yuk;
  yuk.reserve(len < 24000 ? len : 24000);
  unsigned long okunan = 0, b2 = millis();
  while (okunan < len && millis() - b2 < 5000) {
    if (c.available()) { char ch = (char)c.read(); if (okunan < 24000) yuk += ch; okunan++; }
    else if (!c.connected()) break;
    else delay(2);
  }
  return yuk;
}

void tvYaz(WiFiClientSecure& c, const String& js) {
  int n = js.length();
  uint8_t mask[4] = {0x21, 0x43, 0x65, 0x87};
  c.write((uint8_t)0x81);
  if (n < 126) c.write((uint8_t)(0x80 | n));
  else { c.write((uint8_t)(0x80 | 126)); c.write((uint8_t)(n >> 8)); c.write((uint8_t)(n & 0xFF)); }
  c.write(mask, 4);
  uint8_t parca[128];
  int k = 0;
  for (int i = 0; i < n; i++) {
    parca[k++] = (uint8_t)(js[i] ^ mask[i % 4]);
    if (k == 128) { c.write(parca, k); k = 0; }
  }
  if (k) c.write(parca, k);
}

bool gTvUyanabilir = true;   // sesi/oynat tuslari TV'yi uyandirmasin

// TV'nin Wi-Fi MAC adresini bir kez ogrenir (uyandirma icin)
void tvMacOgren(const String& ip) {
  HTTPClient http;
  http.setTimeout(4000);
  if (!http.begin(String("http://") + ip + ":8001/api/v2/")) return;
  int kod = http.GET();
  String r = (kod == 200) ? http.getString() : String("");
  http.end();
  String mac = degerBul(r, "wifiMac");
  if (mac.length() == 17) { nvsYaz("tvmac", mac); Serial.println(String("TV MAC ogrenildi: ") + mac); }
}

// Wake-on-LAN: TV beklemedeyse uyandirir
void tvMagicPaket() {
  String m = nvsOku("tvmac");
  if (m.length() != 17) return;
  uint8_t mac[6];
  for (int i = 0; i < 6; i++) mac[i] = (uint8_t)strtol(m.substring(i * 3, i * 3 + 2).c_str(), NULL, 16);
  uint8_t paket[102];
  memset(paket, 0xFF, 6);
  for (int i = 0; i < 16; i++) memcpy(paket + 6 + i * 6, mac, 6);
  WiFiUDP udp;
  for (int k = 0; k < 3; k++) {
    udp.beginPacket(WiFi.broadcastIP(), 9);
    udp.write(paket, sizeof(paket));
    udp.endPacket();
    udp.beginPacket(IPAddress(255, 255, 255, 255), 9);
    udp.write(paket, sizeof(paket));
    udp.endPacket();
    delay(120);
  }
}

// TV kapaliysa uyandirip WebSocket portu acilana kadar bekler (en fazla ~40 sn)
bool tvUyandirBekle(WiFiClientSecure& c, const String& ip) {
  if (!gTvUyanabilir || nvsOku("tvmac").length() != 17) return false;
  Serial.println("TV kapali gorunuyor, uyandiriyorum...");
  tvMagicPaket();
  unsigned long b = millis();
  while (millis() - b < 40000UL) {
    delay(3000);
    if (millis() - b > 12000UL && (millis() - b) % 9000UL < 3100UL) tvMagicPaket();
    if (c.connect(ip.c_str(), 8002)) { delay(4000); return true; }
  }
  return false;
}

bool tvBaglan(WiFiClientSecure& c) {
  String ip = nvsOku("tvip");
  if (ip.length() < 7) return false;
  c.setInsecure();
  c.setTimeout(6);
  if (!c.connect(ip.c_str(), 8002)) {
    if (!tvUyandirBekle(c, ip)) return false;
  }
  String tok = nvsOku("tvtoken");
  String yol = "/api/v2/channels/samsung.remote.control?name=RmluZHVr";
  if (tok.length()) yol += String("&token=") + tok;
  c.print(String("GET ") + yol + " HTTP/1.1\r\nHost: " + ip + ":8002\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n\r\n");
  String baslik;
  unsigned long b = millis();
  while (millis() - b < 6000) {
    if (c.available()) {
      baslik += (char)c.read();
      if (baslik.endsWith("\r\n\r\n")) break;
    } else if (!c.connected()) break;
    else delay(5);
  }
  if (baslik.indexOf(" 101") < 0) { c.stop(); return false; }
  unsigned long bekle = tok.length() ? 4000UL : 20000UL;
  b = millis();
  while (millis() - b < bekle) {
    String yuk = tvCerceveOku(c, 500);
    if (yuk.length() == 0) { if (!c.connected()) break; continue; }
    if (yuk.indexOf("ms.channel.connect") >= 0) {
      String t2 = degerBul(yuk, "token");
      if (t2.length() > 3 && t2 != tok) nvsYaz("tvtoken", t2);
      if (nvsOku("tvmac").length() != 17) tvMacOgren(ip);
      return true;
    }
    if (yuk.indexOf("ms.channel.timeOut") >= 0 || yuk.indexOf("unauthorized") >= 0) break;
  }
  c.stop();
  return false;
}

bool tvTus(String tus) {
  tus = sadelestir(tus);
  tus.toUpperCase();
  for (unsigned int i = 0; i < tus.length(); i++) if (!(isalnum(tus[i]) || tus[i] == '_')) return false;
  gTvUyanabilir = !(tus.startsWith("KEY_VOL") || tus == "KEY_MUTE" || tus == "KEY_PLAY" || tus == "KEY_PAUSE" || tus == "KEY_FF" || tus == "KEY_REWIND" || tus == "KEY_STOP");
  WiFiClientSecure c;
  bool bagli = tvBaglan(c);
  gTvUyanabilir = true;
  if (!bagli) return false;
  tvYaz(c, String("{\"method\":\"ms.remote.control\",\"params\":{\"Cmd\":\"Click\",\"DataOfCmd\":\"") + tus + "\",\"Option\":\"false\",\"TypeOfRemote\":\"SendRemoteKey\"}}");
  delay(400);
  c.stop();
  return true;
}

String jsonKacir(const String& s) {
  String o;
  for (unsigned int i = 0; i < s.length(); i++) {
    char ch = s[i];
    if (ch == '"' || ch == '\\') { o += '\\'; o += ch; }
    else if ((uint8_t)ch >= 32) o += ch;
  }
  return o;
}

// Uygulamayi WebSocket ile acar; metaTag doluysa icindeki sayfaya (film) gider. Olmazsa REST ile dener.
bool tvAppBaslat(const String& appId, const String& meta) {
  WiFiClientSecure c;
  if (tvBaglan(c)) {
    tvYaz(c, String("{\"method\":\"ms.channel.emit\",\"params\":{\"event\":\"ed.apps.launch\",\"to\":\"host\",\"data\":{\"appId\":\"") + jsonKacir(appId) + "\",\"action_type\":\"DEEP_LINK\",\"metaTag\":\"" + jsonKacir(meta) + "\"}}}");
    String y = tvCerceveOku(c, 2500);
    c.stop();
    Serial.println(String("TV uygulama ac (") + appId + "): " + y.substring(0, 120));
    if (y.indexOf("\"data\":200") >= 0 || y.indexOf("\"data\":true") >= 0 || y.length() == 0) return true;
  }
  String ip = nvsOku("tvip");
  HTTPClient http;
  http.setTimeout(5000);
  if (!http.begin(String("http://") + ip + ":8001/api/v2/applications/" + appId)) return false;
  int kod = http.POST("");
  http.end();
  return kod == 200 || kod == 201;
}

// TV'deki yuklu uygulamalari ogrenir, NVS'e "appId ad" satirlari olarak yazar
int tvAppleriOgren() {
  WiFiClientSecure c;
  if (!tvBaglan(c)) return 0;
  tvYaz(c, "{\"method\":\"ms.channel.emit\",\"params\":{\"event\":\"ed.installedApp.get\",\"to\":\"host\"}}");
  String y;
  unsigned long b = millis();
  while (millis() - b < 6000) {
    y = tvCerceveOku(c, 1500);
    if (y.indexOf("ed.installedApp.get") >= 0) break;
  }
  c.stop();
  String liste;
  int n = 0, p = 0;
  while (n < 60) {
    int a = y.indexOf("\"appId\":\"", p);
    if (a < 0) break;
    a += 9;
    int e = y.indexOf('"', a);
    if (e < 0) break;
    String id = y.substring(a, e);
    int sonraki = y.indexOf("\"appId\":\"", e);
    int na = y.indexOf("\"name\":\"", e);
    String ad;
    if (na >= 0 && (sonraki < 0 || na < sonraki)) { na += 8; int ne = y.indexOf('"', na); if (ne > na) ad = y.substring(na, ne); }
    p = e;
    ad = sadelestir(ad);
    if (id.length() && ad.length()) { liste += id + " " + ad + "\n"; n++; }
  }
  if (n > 0) nvsYaz("tvapps", liste);
  Serial.println(String("TV uygulamalari: ") + n);
  return n;
}

String platformAnahtar(String ad) {
  ad = sadelestir(ad);
  ad.replace("amazon ", "");
  ad.replace(" standard with ads", "");
  ad.replace(" with ads", "");
  ad.replace("+", " plus");
  ad.trim();
  if (ad.startsWith("apple tv")) return "apple tv";
  if (ad.startsWith("prime video")) return "prime";
  if (ad.startsWith("hbo max") || ad == "max") return "max";
  if (ad.startsWith("tv plus") || ad.startsWith("tv+")) return "tv+";
  int sp = ad.indexOf(' ');
  String ilk = (sp > 0) ? ad.substring(0, sp) : ad;
  return ilk;
}

// Platform adina gore TV'deki uygulamanin kimligini bulur
String tvAppBul(const String& platform) {
  String k = platformAnahtar(platform);
  if (k.length() < 2) return "";
  for (int deneme = 0; deneme < 2; deneme++) {
    String s = nvsOku("tvapps");
    int bas = 0;
    while (bas < (int)s.length()) {
      int e = s.indexOf('\n', bas);
      if (e < 0) e = s.length();
      String satir = s.substring(bas, e);
      bas = e + 1;
      int sp = satir.indexOf(' ');
      if (sp <= 0) continue;
      String ad = satir.substring(sp + 1);
      if (ad.indexOf(k) >= 0 || (k == "max" && ad.indexOf("hbo") >= 0)) return satir.substring(0, sp);
    }
    if (deneme == 0 && tvAppleriOgren() == 0) break;
  }
  if (k == "netflix") return "3201907018807";
  if (k == "youtube") return "111299001912";
  if (k == "prime") return "3201910019365";
  if (k == "disney") return "3201901017640";
  if (k == "apple tv") return "3201807016597";
  return "";
}

bool tvUygulama(const String& ad) {
  String id = tvAppBul(ad);
  if (id.length() == 0) return false;
  return tvAppBaslat(id, "");
}

// JustWatch'ta filmi arar (Turkiye katalogu)
String jwAra(const String& ad) {
  WiFiClientSecure c;
  c.setInsecure();
  HTTPClient http;
  http.setTimeout(9000);
  if (!http.begin(c, "https://apis.justwatch.com/graphql")) return "";
  http.addHeader("Content-Type", "application/json");
  http.addHeader("User-Agent", "Mozilla/5.0");
  String q = String("{\"query\":\"query S($f: TitleFilter!, $c: Country!, $l: Language!, $n: Int!) { popularTitles(country: $c, filter: $f, first: $n) { edges { node { objectType content(country: $c, language: $l) { title originalReleaseYear } offers(country: $c, platform: WEB) { monetizationType standardWebURL package { clearName technicalName } } } } } }\",\"variables\":{\"f\":{\"searchQuery\":\"") + jsonKacir(ad) + "\"},\"c\":\"TR\",\"l\":\"tr\",\"n\":1}}";
  int kod = http.POST(q);
  String r = (kod == 200) ? http.getString() : String("");
  http.end();
  Serial.println(String("JustWatch -> HTTP ") + kod + ", " + r.length() + " bayt");
  return r;
}

void tvFilmAc(const String& ad0) {
  String ad = ad0;
  ad.trim();
  if (ad.length() == 0) return;
  if (nvsOku("tvip").length() < 7) { konus("Televizyon tanimli degil."); return; }
  String r = jwAra(ad);
  if (r.indexOf("\"edges\"") < 0) { konus("Film arama servisine ulasamadim."); return; }
  String baslik = degerBul(r, "title");
  if (baslik.length() == 0) { konus(ad + " filmini bulamadim."); return; }
  // Teklifleri tek tek gez
  String nfxId = "", ytId = "", aboneUrl = "", abonePlat = "", kiraPlat = "";
  int p = 0;
  while (true) {
    int a = r.indexOf("\"monetizationType\":\"", p);
    if (a < 0) break;
    int sonraki = r.indexOf("\"monetizationType\":\"", a + 10);
    String par = (sonraki < 0) ? r.substring(a) : r.substring(a, sonraki);
    p = a + 10;
    String tur = degerBul(par, "monetizationType");
    String url = degerBul(par, "standardWebURL");
    String plat = degerBul(par, "clearName");
    bool abone = (tur == "FLATRATE" || tur == "FREE" || tur == "ADS");
    int t = url.indexOf("netflix.com/title/");
    if (abone && t >= 0 && nfxId.length() == 0) {
      int s = t + 18, e = s;
      while (e < (int)url.length() && isdigit(url[e])) e++;
      nfxId = url.substring(s, e);
    }
    int y = url.indexOf("watch?v=");
    if (abone && y >= 0 && ytId.length() == 0) ytId = url.substring(y + 8, y + 19);
    if (abone && abonePlat.length() == 0 && t < 0) { abonePlat = plat; aboneUrl = url; }
    if (!abone && kiraPlat.length() == 0) kiraPlat = plat;
  }
  bool ok = false;
  String mesaj;
  if (nfxId.length()) {
    ok = tvAppBaslat("3201907018807", String("m=") + nfxId + "&source_type_payload=groupIndex%3D2%26tileIndex%3D6%26action%3Dmdp%26movieId%3D" + nfxId);
    mesaj = baslik + " Netflix'te, televizyonda aciyorum.";
  } else if (ytId.length()) {
    ok = tvAppBaslat(tvAppBul("youtube"), String("v=") + ytId);
    mesaj = baslik + " YouTube'da ucretsiz, aciyorum.";
  } else if (abonePlat.length()) {
    String id = tvAppBul(abonePlat);
    if (id.length()) {
      ok = tvAppBaslat(id, aboneUrl);
      mesaj = baslik + ", " + abonePlat + " platformunda var. Uygulamayi actim, film dogrudan acilmazsa aramaya adini yazman yeterli.";
    } else {
      mesaj = baslik + ", " + abonePlat + " platformunda var ama bu uygulama televizyonda yuklu degil.";
      ok = true;
    }
  } else if (kiraPlat.length()) {
    mesaj = baslik + " abonelikle izlenebilecek bir platformda yok, sadece " + kiraPlat + " uzerinden kiralanabiliyor ya da satin alinabiliyor.";
    ok = true;
  } else {
    mesaj = baslik + " su an Turkiye'deki dijital platformlarin hicbirinde gorunmuyor.";
    ok = true;
  }
  if (!ok) mesaj = "Televizyona ulasamadim. Televizyon acik mi?";
  gUyku = false;
  konus(mesaj);
  bildirBaslik("TV", mesaj, 2, "tv");
}

// Sinema Odasi TV uygulamasindan gelen istek: platformun film adresiyle dogrudan acar, olmazsa JustWatch ile arar
// --- Platforma gore filmi acma: her platform icin secilebilir yontem, sonucu soyler ---
String gTvSoz = "";
String gSonFilm = "";

String tvLinkId(const String& url) {
  String u = url;
  int g = u.indexOf("gti=");
  if (g >= 0) { int e = u.indexOf('&', g); return u.substring(g + 4, e < 0 ? u.length() : e); }
  int q = u.indexOf('?');
  if (q >= 0) u = u.substring(0, q);
  while (u.endsWith("/")) u.remove(u.length() - 1);
  int sl = u.lastIndexOf('/');
  return sl >= 0 ? u.substring(sl + 1) : u;
}

bool dialBaslat(const String& ad, const String& govde) {
  String ip = nvsOku("tvip");
  if (ip.length() < 7) return false;
  HTTPClient http;
  http.setTimeout(5000);
  if (!http.begin(String("http://") + ip + ":8080/ws/apps/" + ad)) return false;
  http.addHeader("Content-Type", "text/plain; charset=utf-8");
  int kod = http.POST(govde);
  http.end();
  Serial.println(String("DIAL ") + ad + ": " + kod);
  return kod == 200 || kod == 201;
}

// TV'de acik bir yazi kutusu varsa (arama gibi) metni yazar
bool tvYazi(const String& metin) {
  if (metin.length() == 0) return false;
  WiFiClientSecure c;
  if (!tvBaglan(c)) return false;
  int n = metin.length();
  char* b = (char*)malloc(b64Uzunluk(n) + 2);
  if (!b) { c.stop(); return false; }
  int o = b64Kodla((const uint8_t*)metin.c_str(), n, b);
  b[o] = 0;
  tvYaz(c, String("{\"method\":\"ms.remote.control\",\"params\":{\"Cmd\":\"") + b + "\",\"DataOfCmd\":\"base64\",\"TypeOfRemote\":\"SendInputString\"}}");
  free(b);
  delay(300);
  tvYaz(c, "{\"method\":\"ms.remote.control\",\"params\":{\"TypeOfRemote\":\"SendInputEnd\"}}");
  delay(300);
  c.stop();
  return true;
}

// yontem 1 = Tizen derin bag (tam adres), 2 = sadece kimlik, 3 = DIAL, 4 = sadece uygulamayi ac
bool tvPlatformAc(const String& plat, const String& url, const String& film) {
  String k = platformAnahtar(plat);
  String id = tvAppBul(plat);
  if (id.length() == 0) return false;
  String y = nvsOku((String("dl_") + k.substring(0, 10)).c_str());
  int yon = y.length() ? y.toInt() : 1;
  bool ok = false;
  if (yon == 2) ok = tvAppBaslat(id, tvLinkId(url));
  else if (yon == 3) {
    String ad = (k == "prime") ? "AmazonInstantVideo" : (k == "disney") ? "DisneyPlus" : "";
    if (ad.length()) ok = dialBaslat(ad, url);
    if (!ok) ok = tvAppBaslat(id, url);
  }
  else if (yon == 4) ok = tvAppBaslat(id, "");
  else ok = tvAppBaslat(id, url);
  if (ok) gTvSoz = plat + " acildi. Film dogrudan acilmazsa aramayi ac ve bana yaz de, adini ben yazarim.";
  Serial.println(String("Platform ac ") + k + " yontem " + yon + ": " + (ok ? "ok" : "hata"));
  return ok;
}

void tvSozKontrol() {
  if (gTvSoz.length() == 0) return;
  String m = gTvSoz;
  gTvSoz = "";
  konus(m);
}

void tvUrlAc(const String& plat, const String& url, const String& film) {
  if (film.length()) gSonFilm = film;
  bool ok = false;
  int t = url.indexOf("netflix.com/title/");
  int y = url.indexOf("watch?v=");
  if (t >= 0) {
    int s0 = t + 18, e = s0;
    while (e < (int)url.length() && isdigit(url[e])) e++;
    String id = url.substring(s0, e);
    if (id.length()) ok = tvAppBaslat("3201907018807", String("m=") + id + "&source_type_payload=groupIndex%3D2%26tileIndex%3D6%26action%3Dmdp%26movieId%3D" + id);
    if (ok) gTvSoz = film + ", Netflix'te aciliyor.";
  } else if (y >= 0 && url.length() >= (unsigned int)(y + 19)) {
    ok = tvAppBaslat(tvAppBul("youtube"), String("v=") + url.substring(y + 8, y + 19));
  } else if (plat.length()) {
    ok = tvPlatformAc(plat, url, film);
  }
  if (!ok && film.length()) { tvFilmAc(film); return; }
  if (!ok) bildirBaslik("TV", String("TV'de acilamadi: ") + plat, 3, "tv");
}

// Kanal: ya yukari/asagi ya da numara (once rakamlar, sonra Tamam)
bool tvKanal(String d) {
  d = sadelestir(d);
  d.trim();
  WiFiClientSecure c;
  if (!tvBaglan(c)) return false;
  String dizi[8];
  int n = 0;
  if (d == "+" || d.indexOf("yukari") >= 0 || d.indexOf("ileri") >= 0 || d.indexOf("sonraki") >= 0) dizi[n++] = "KEY_CHUP";
  else if (d == "-" || d.indexOf("asagi") >= 0 || d.indexOf("geri") >= 0 || d.indexOf("onceki") >= 0) dizi[n++] = "KEY_CHDOWN";
  else {
    for (unsigned int i = 0; i < d.length() && n < 6; i++) if (d[i] >= '0' && d[i] <= '9') dizi[n++] = String("KEY_") + d[i];
    if (n == 0) { c.stop(); return false; }
    dizi[n++] = "KEY_ENTER";
  }
  for (int i = 0; i < n; i++) {
    tvYaz(c, String("{\"method\":\"ms.remote.control\",\"params\":{\"Cmd\":\"Click\",\"DataOfCmd\":\"") + dizi[i] + "\",\"Option\":\"false\",\"TypeOfRemote\":\"SendRemoteKey\"}}");
    delay(450);
  }
  c.stop();
  return true;
}

// TV'yi uyandir (kapaliysa); aciksa bir sey yapmaz
bool tvAc() {
  String ip = nvsOku("tvip");
  if (ip.length() < 7) return false;
  WiFiClientSecure c;
  c.setInsecure();
  c.setTimeout(4);
  if (c.connect(ip.c_str(), 8002)) { c.stop(); return true; }
  gTvUyanabilir = true;
  bool ok = tvUyandirBekle(c, ip);
  c.stop();
  return ok;
}

// TV'nin tarayicisinda bir web sayfasi acar ("sinema" = Demet'in Sinema Odasi)
bool tvWeb(String adres) {
  adres.trim();
  if (sadelestir(adres).indexOf("sinema") == 0) adres = "https://okuzey3-cmyk.github.io/Sinema-Odas-/";
  if (!adres.startsWith("https://")) return false;
  return tvAppBaslat("org.tizen.browser", adres);
}

void tvKomut(const String& ar) {
  int c = ar.indexOf('|');
  if (c <= 0) return;
  String tur = ar.substring(0, c);
  String deger = ar.substring(c + 1);
  tur.trim();
  bool ok = false;
  if (tur == "tus") ok = tvTus(deger);
  else if (tur == "kanal") ok = tvKanal(deger);
  else if (tur == "ac") ok = tvAc();
  else if (tur == "web") ok = tvWeb(deger);
  else if (tur == "uygulama") { if (sadelestir(deger).indexOf("sinema") >= 0) ok = tvWeb("sinema"); else ok = tvUygulama(deger); }
  else if (tur == "film") { tvFilmAc(deger); return; }
  else if (tur == "yaz") { String m = deger; m.trim(); if (m.length() == 0) m = gSonFilm; ok = tvYazi(m); }
  else if (tur == "yontem") {
    String d = sadelestir(deger);
    int sp = d.lastIndexOf(' ');
    if (sp > 0) { nvsYaz((String("dl_") + platformAnahtar(d.substring(0, sp)).substring(0, 10)).c_str(), d.substring(sp + 1)); ok = true; }
  }
  if (!ok) bildir("TV komutu gonderilemedi (IP, TV acik mi, izin verildi mi bak)");
}

// --- Wi-Fi kurulum agi: telefon Finduk-Kurulum agina baglanir, 192.168.4.1/kurulum sayfasini acar ---
void kurulumSayfa() {
  int n = WiFi.scanNetworks();
  String h = "<!doctype html><html><head><meta charset=utf-8><meta name=viewport content='width=device-width,initial-scale=1'><title>Finduk kurulum</title></head><body style='font-family:-apple-system,sans-serif;margin:16px'><h2>Finduk Wi-Fi kurulumu</h2><form action=/kaydet method=post><select name=wifiad style='font-size:18px;width:100%'>";
  for (int i = 0; i < n && i < 15; i++) h += String("<option>") + htmlKacir(WiFi.SSID(i)) + "</option>";
  h += "</select><br><input type=password name=wifisifre placeholder='Wi-Fi sifresi' style='font-size:18px;width:100%;margin:8px 0'><br><button style='font-size:18px;padding:8px'>Kaydet</button></form></body></html>";
  webSunucu.send(200, "text/html; charset=utf-8", h);
}

void kurulumKaydet() {
  String a = webSunucu.arg("wifiad");
  String s = webSunucu.arg("wifisifre");
  if (a.length() == 0) { webSunucu.send(400, "text/plain", "Ag adi bos"); return; }
  nvsYaz("wifi", a);
  nvsYaz("sifre", s);
  webSunucu.send(200, "text/html; charset=utf-8", "<p>Kaydedildi. Finduk yeniden basliyor.</p>");
  delay(1500);
  ESP.restart();
}

void kurulumAP() {
  Serial.println("Wi-Fi kurulum modu: Finduk-Kurulum agi acildi, 192.168.4.1/kurulum");
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("Finduk-Kurulum");
  webSunucu.on("/kurulum", kurulumSayfa);
  webSunucu.on("/kaydet", HTTP_POST, kurulumKaydet);
  if (!gWebVar) webSunucu.begin();
  unsigned long b = millis();
  while (millis() - b < 600000UL) {
    if (!gWebVar) webSunucu.handleClient();
    delay(5);
  }
  if (WiFi.status() == WL_CONNECTED) { WiFi.softAPdisconnect(true); return; }
  ESP.restart();
}

// --- Paket 28 komutlari ---
bool paket28Komut(const String& ad, const String& ar) {
  if (ad == "espri") satirEkle("espri", ar, 20);
  else if (ad == "esprisil") { if (ar == "hepsi") nvsYaz("espri", ""); else satirSil("espri", ar); }
  else if (ad == "macera") maceraIsle(ar);
  else if (ad == "karakter") { if (sadelestir(ar).indexOf("kapat") == 0) nvsYaz("karakter", ""); else nvsYaz("karakter", temizMetin(ar, 120)); }
  else if (ad == "kisi") { gKisi = (sadelestir(ar).indexOf("demet") >= 0) ? "demet" : ""; gKisiBitis = millis() + 20UL * 60000UL; }
  else if (ad == "misafir") { if (sadelestir(ar).indexOf("kapat") >= 0) gMisafir = false; else { gMisafir = true; gMisafirBitis = millis() + 4UL * 3600000UL; } }
  else if (ad == "dil") {
    if (sadelestir(ar).indexOf("kapat") >= 0) nvsYaz("dil_modu", "");
    else { int c = ar.indexOf('|'); String s = (c >= 0) ? ar.substring(c + 1) : String(""); s = temizMetin(s, 20); if (s.length() == 0) s = "baslangic"; nvsYaz("dil_modu", s); }
  }
  else if (ad == "tarif") tarifKur(ar);
  else if (ad == "vardiya") vardiyaKur(ar);
  else if (ad == "su") suKur(ar);
  else if (ad == "arac") { int c = ar.indexOf('|'); if (c > 0 && ar.length() >= c + 11 && ar[c + 5] == '-') satirEkle("arac", ar.substring(c + 1, c + 11) + " " + ar.substring(0, c), 20); }
  else if (ad == "aracsil") satirSil("arac", ar);
  else if (ad == "yemekplan") {
    int bas = 0;
    while (bas < (int)ar.length()) {
      int e = ar.indexOf(';', bas);
      if (e < 0) e = ar.length();
      String t = ar.substring(bas, e);
      t.trim();
      bas = e + 1;
      if (t.length()) satirEkle("yemekplan", t, 14);
    }
  }
  else if (ad == "yemekplansil") { if (ar == "hepsi") nvsYaz("yemekplan", ""); else satirSil("yemekplan", ar); }
  else if (ad == "kasa") kasaKaydet(ar);
  else if (ad == "kasasil") satirSil("kasa", sadelestir(ar));
  else if (ad == "mac") macKur(ar);
  else if (ad == "cihaz") { if (!cihazKomut(ar)) bildir("Cihaz komutu calismadi (kayitli mi, acik mi bak)"); }
  else if (ad == "tv") tvKomut(ar);
  else if (ad == "tvfilm") tvFilmAc(ar);
  else if (ad == "kurulum") kurulumAP();
  else return false;
  return true;
}

// Dakikalik/periyodik paket 28 isleri (uyku ve uyanik dongusunde cagrilir)
void paket28Periyodik() {
  if (gMisafir && millis() > gMisafirBitis) gMisafir = false;
  if (gKisi.length() && millis() > gKisiBitis) gKisi = "";
  kendiKonus();
  robotGunlukKontrol();
  yilOnceKontrol();
  ozelModKontrol();
  suKontrol();
  aracKontrol();
  vardiyaKontrol();
  macKontrol();
  gunMaciKontrol();
  tvSozKontrol();
}
// ===== BUYUK PAKET 28 BITIS =====

// ===== BUYUK PAKET 29: CANLILIK VE GUVENLIK PAKETI BASLA =====
// --- Geri donus guvenligi: surum ust uste 3 kez cokerse (panic/watchdog) eski surume doner ---
void geriDonDogrula() {
  if (gGeriDogrulandi || millis() < 45000UL) return;
  gGeriDogrulandi = true;
#ifdef CONFIG_APP_ROLLBACK_ENABLE
  esp_ota_mark_app_valid_cancel_rollback();
#endif
  nvsYaz("bekle", "");
  nvsYaz("cokme", "0");
}

void acilisKontrol() {
  esp_reset_reason_t r = esp_reset_reason();
  bool coktu = (r == ESP_RST_PANIC || r == ESP_RST_INT_WDT || r == ESP_RST_TASK_WDT || r == ESP_RST_WDT);
  String bekle = nvsOku("bekle");
  int c = nvsOku("cokme").toInt();
  if (bekle.length() == 0) c = 0;
  else if (coktu) c++;
  nvsYaz("cokme", String(c));
  nvsYaz("bekle", String(FW_SURUM));
  if (c < 3) return;
  const esp_partition_t* diger = esp_ota_get_next_update_partition(NULL);
  esp_app_desc_t d;
  esp_ota_img_states_t st;
  if (!diger || esp_ota_get_partition_description(diger, &d) != ESP_OK) return;
  if (esp_ota_get_state_partition(diger, &st) == ESP_OK && (st == ESP_OTA_IMG_INVALID || st == ESP_OTA_IMG_ABORTED)) return;
  Serial.println("Bu surum surekli cokuyor, eski surume donuyorum.");
  nvsYaz("kotu_s", String(FW_SURUM));
  nvsYaz("cokme", "0");
  nvsYaz("bekle", "");
  if (esp_ota_set_boot_partition(diger) == ESP_OK) {
    delay(300);
    ESP.restart();
  }
}

void kotuSurumBildir() {
  String k = nvsOku("kotu_s");
  if (k.length() && nvsOku("kotu_bil") != k) {
    nvsYaz("kotu_bil", k);
    bildir(String("Surum ") + k + " surekli coktu, eski surume (" + FW_SURUM + ") dondum. Daha yeni bir surum gelene kadar guncelleme beklenecek.");
  }
}

// --- Hiz hissi: sesi duyunca kisa, yumusak bir "hmm" (internetsiz, kodla uretilir) ---
void dusunmeSesi() {
  if (nvsOku("dolgu") == "0") return;
  int vol = sesYuzdeSimdi();
  i2s_set_sample_rates(I2S_NUM_1, 24000);
  const int toplam = 24000 * 420 / 1000;
  static int16_t b[256];
  float faz = 0;
  int yaz = 0;
  while (yaz < toplam) {
    int n = toplam - yaz;
    if (n > 256) n = 256;
    for (int i = 0; i < n; i++) {
      int k = yaz + i;
      float t = (float)k / (float)toplam;
      faz += 2.0f * 3.14159265f * (205.0f + 45.0f * t) / 24000.0f;   // hafif yukselen "hmm"
      float zarf = 1.0f;
      if (k < 1800) zarf = (float)k / 1800.0f;
      if (toplam - k < 2400) zarf = (float)(toplam - k) / 2400.0f;
      float v = 0;
      for (int h = 1; h <= 6; h++) v += sinf((float)h * faz) / ((float)h * sqrtf((float)h));
      b[i] = (int16_t)(v * 3600.0f * zarf * (float)vol / 100.0f);
    }
    size_t w = 0;
    i2s_write(I2S_NUM_1, b, n * 2, &w, portMAX_DELAY);
    yaz += n;
  }
  sus(80);
}

// --- Ruh hali: saate ve gune gore degisen huy ---
String ruhHaliMetni() {
  long now = simdiEpoch();
  int h = 12;
  if (now != 0) { time_t tt = (time_t)now; struct tm z; localtime_r(&tt, &z); h = z.tm_hour; }
  String s;
  if (h >= 5 && h < 11) s = "Sabah vakti: biraz uykulu, yavas ve nazlisin.";
  else if (h < 17) s = "Gun ortasi: enerjik, merakli ve saka havasindasin.";
  else if (h < 21) s = "Aksam: rahat, sicak ve keyiflisin.";
  else if (h < 23) s = "Gun bitiyor: sakin ve yumusaksin, kisa konusursun.";
  else s = "Gece: fisildar gibi, cok kisa ve uykulu konusursun, istersen uyku vaktini hatirlat.";
  String g = bugunTarih();
  int hs = 0;
  for (unsigned int i = 0; i < g.length(); i++) hs += (unsigned char)g[i];
  const char* huy[] = {
    " Bugun ozellikle merakli bir gundesin, ara sira bir soru sor.",
    " Bugun saka havasindasin, hafif esprili ol.",
    " Bugun sakin ve dusunceli bir gundesin.",
    " Bugun neseli ve hareketli bir gundesin.",
    " Bugun biraz miskin ve uykucusun ama sevimlisin."
  };
  s += huy[hs % 5];
  return s;
}

String paket29Sistem() {
  String s = String(" RUH HALIN: ") + ruhHaliMetni() + " Bunu sadece tonuna yansit, rol yaptigini ya da ruh halini aciklama, kisa kal.";
  s += " NOBET MODU: Kullanici evden cikiyorum, ev bos ya da nobet modu derse [nobet:ac] yaz (hassas isterse [nobet:ac|hassas]), eve donunce ya da kapat derse [nobet:kapat]. Nobette robot sessizce uyur, yuksek ses duyarsa duydugunu yaziya cevirip telefona haber verir. Durust ol: bu sadece yuksek seslere bakar, gercek bir guvenlik alarmi degildir. Adini soyleyince nobet kapanir.";
  s += " TAKIM: Kullanici tuttugu takimi soylerse [takim:ekle|Fenerbahce], birakirsa [takim:sil|Fenerbahce] yaz (ASCII, tek takim). Varsayilan takip edilen takim Galatasaray. Robot takimlarin bugun/yarin macini kendisi internetten bulup aksamlari sormadan haber verir.";
  s += " DUSUNME SESI: Kullanici hmm sesini (dusunme sesi) istemezse [dolgu:kapat], tekrar isterse [dolgu:ac] yaz.";
  return s;
}

// --- Uyku masali ve ek bilgi ---
String paket29Bilgi(const String& sd) {
  String o = "";
  if (sd.indexOf("masal") >= 0 || sd.indexOf("uyutur") >= 0 || sd.indexOf("uyku hikaye") >= 0) {
    gMasalBitis = millis() + 15UL * 60000UL;
    o += "UYKU MASALI: Kullanici uyku masali istiyor. Cok yavas, sakin ve yumusak tonla, kisa cumlelerle 6-8 cumlelik huzurlu bir masal anlat (korkutucu olay yok; hayvanlar, yildizlar, deniz gibi sakin ogeler). Bu modda 2 cumle siniri yoktur. Sonda fisildar gibi iyi geceler de ve cevabin sonuna [uyu:1] yaz. ";
  }
  return o;
}

// --- Emanet mesaji kisi kendini tanitinca hemen ilet (API'siz) ---
bool emanetTeslim(const String& kisi, const String& hitap) {
  String s = nvsOku("emanet");
  if (s.length() == 0) return false;
  String mesaj = "", kalan = "";
  int bas = 0;
  while (bas < (int)s.length()) {
    int e = s.indexOf('\n', bas);
    if (e < 0) e = s.length();
    String satir = s.substring(bas, e);
    bas = e + 1;
    satir.trim();
    if (satir.length() == 0) continue;
    int c = satir.indexOf(':');
    String kime = (c > 0) ? sadelestir(satir.substring(0, c)) : String("");
    if (c > 0 && kime.indexOf(kisi) >= 0) {
      String m = satir.substring(c + 1);
      m.trim();
      mesaj += m + ". ";
    } else {
      kalan += satir + "\n";
    }
  }
  if (mesaj.length() == 0) return false;
  nvsYaz("emanet", kalan);
  konus(hitap + ", sana birakilmis mesaj var. " + mesaj);
  return true;
}

// --- Mutfak eli: adimda "10 dakika" gecerse zamanlayici otomatik kurulur ---
int adimSuresi(const String& t) {
  String s = sadelestir(t);
  int n = s.length();
  for (int i = 0; i < n; i++) {
    if (s[i] >= '0' && s[i] <= '9') {
      int j = i, v = 0;
      while (j < n && s[j] >= '0' && s[j] <= '9') { v = v * 10 + (s[j] - '0'); j++; }
      while (j < n && s[j] == ' ') j++;
      if (s.startsWith("dakika", j) || s.startsWith("dk", j)) return (v > 0 && v <= 180) ? v : 0;
      i = j;
    }
  }
  return 0;
}

// --- Hos geldin selami (aksam fise takilinca gunde bir kez) ---
void girisSelami() {
  long now = simdiEpoch();
  if (now == 0 || WiFi.status() != WL_CONNECTED) return;
  time_t tt = (time_t)now;
  struct tm z;
  localtime_r(&tt, &z);
  if (z.tm_hour < 15 || z.tm_hour >= 23) return;
  esp_reset_reason_t r = esp_reset_reason();
  if (r != ESP_RST_POWERON && r != ESP_RST_SW) return;
  String g = bugunTarih();
  if (nvsOku("gs_gun") == g) return;
  nvsYaz("gs_gun", g);
  String bilgi = String("HOS GELDIN: Robot az once acildi, ev sahibi eve donmus olabilir. Kisa (en fazla 2 cumle), sicak bir iyi aksamlar ya da hos geldin de. Bekleyen emanet mesaj varsa kime ait oldugunu soyle ama icerigini soyleme: ") + emanetler() + ". Yaklasan ozel gun: " + ozelGunler(7) + ". Kendini tanitma, uzatma.";
  robotKonusur("(Eve hos geldin de)", bilgi);
}

// --- Nobet modu: sessizce dinler, yuksek ses olursa telefona haber verir ---
void nobetOlay() {
  int tepe = kaydet();
  if (gSesliMs < MIN_SESLI_MS) return;
  String duyulan = "";
  if (WiFi.status() == WL_CONNECTED && gNobetStt < 40) {
    gNobetStt++;
    String ham = geminiSesiYaziyaCevir();
    duyulan = degerBul(ham, "text");
    if (duyulan == "BOS") duyulan = "";
  }
  if (duyulan.length() && adiGeciyor(duyulan)) {
    gNobet = false;
    gUyku = false;
    gMood = M_MUTLU;
    gMoodBitis = millis() + 3000;
    konus("Hos geldin! Nobeti kapattim.");
    return;
  }
  if (millis() - gNobetSon < 120000UL || gNobetSay >= 20) return;
  gNobetSon = millis();
  gNobetSay++;
  String m = String("Evde yuksek ses var (seviye ") + tepe + ", " + (gSesliMs / 1000) + " sn). Saat " + zamanYazi() + ".";
  if (duyulan.length()) m += String(" Duyulan: ") + duyulan.substring(0, 80);
  bildirBaslik("Finduk nobetci", m, 4, "warning");
}

void nobetDongusu() {
  if (millis() > gNobetBitis) {
    gNobet = false;
    bildir("Nobet suresi doldu, nobet modu kapandi.");
    return;
  }
  Serial.println("Nobet: sessizce dinliyorum...");
  if (alarmCal()) return;
  if (bekleSes(60000UL, gNobetEsik)) {
    nobetOlay();
    return;
  }
  telefonMesajiIsle();
  if (millis() - sonGuncelKontrol > GUNCEL_ARALIK) guncellemeKontrol();
}

// --- Gunun maci: favori takimlarin macini internetten bulur, sormadan haber verir ---
long utcEpoch(int y, int m, int d, int H, int M) {
  y -= (m <= 2);
  long era = (y >= 0 ? y : y - 399) / 400;
  long yoe = y - era * 400;
  long doy = (153L * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  long gun = era * 146097L + doe - 719468L;
  return gun * 86400L + H * 3600L + M * 60L;
}

String takimIdBul(const String& takim) {
  String anahtar = String("tid_");
  for (unsigned int i = 0; i < takim.length() && anahtar.length() < 14; i++) anahtar += isalnum(takim[i]) ? takim[i] : '_';
  String id = nvsOku(anahtar.c_str());
  if (id.length()) return id;
  String q = takim;
  q.replace(" ", "%20");
  String yol = String("/api/v1/json/3/searchteams.php?t=") + q;
  String r = httpsGetGovde("www.thesportsdb.com", yol.c_str(), 9000);
  id = degerBul(r, "idTeam");
  if (id.length() == 0) {   // arama bulamazsa Super Lig listelerinden takimi tani
    const char* yollar[2] = {"/api/v1/json/3/eventsnextleague.php?id=4339", "/api/v1/json/3/eventspastleague.php?id=4339"};
    for (int k = 0; k < 2 && id.length() == 0; k++) {
      String l = httpsGetGovde("www.thesportsdb.com", yollar[k], 14000);
      int bas = 0;
      while (id.length() == 0) {
        int a = l.indexOf("{\"idEvent\"", bas);
        if (a < 0) break;
        int b = l.indexOf("{\"idEvent\"", a + 10);
        String seg = (b < 0) ? l.substring(a) : l.substring(a, b);
        bas = (b < 0) ? l.length() : b;
        if (sadelestir(degerBul(seg, "strHomeTeam")).indexOf(takim) >= 0) id = degerBul(seg, "idHomeTeam");
        else if (sadelestir(degerBul(seg, "strAwayTeam")).indexOf(takim) >= 0) id = degerBul(seg, "idAwayTeam");
      }
    }
  }
  if (id.length()) nvsYaz(anahtar.c_str(), id);
  return id;
}

// Bir takimin bugun/yarin maclarini bulur (bitenler skorla). Bulduklarini metne ekler.
bool takimMaciBul(const String& takim, long now, String& metin) {
  String id = takimIdBul(takim);
  if (id.length() == 0) return false;
  bool tamam = false;
  const char* yolSon[2] = {"eventsnext.php", "eventslast.php"};
  long bugunNo = (now + 3 * 3600L) / 86400L;
  for (int k = 0; k < 2; k++) {
    String yol = String("/api/v1/json/3/") + yolSon[k] + "?id=" + id;
    String r = httpsGetGovde("www.thesportsdb.com", yol.c_str(), 9000);
    if (r.length() < 30) continue;
    tamam = true;
    int bas = 0;
    while (true) {
      int a = r.indexOf("{\"idEvent\"", bas);
      if (a < 0) break;
      int b = r.indexOf("{\"idEvent\"", a + 10);
      String seg = (b < 0) ? r.substring(a) : r.substring(a, b);
      bas = (b < 0) ? r.length() : b;
      String ts = degerBul(seg, "strTimestamp");
      if (ts.length() < 16) {
        String d0 = degerBul(seg, "dateEvent"), t0 = degerBul(seg, "strTime");
        if (d0.length() < 10 || t0.length() < 5) continue;
        ts = d0 + "T" + t0;
      }
      long t = utcEpoch(ts.substring(0, 4).toInt(), ts.substring(5, 7).toInt(), ts.substring(8, 10).toInt(), ts.substring(11, 13).toInt(), ts.substring(14, 16).toInt());
      long fark = (t + 3 * 3600L) / 86400L - bugunNo;
      if (fark < 0 || fark > 1) continue;
      String ev = degerBul(seg, "strHomeTeam"), dep = degerBul(seg, "strAwayTeam"), lig = degerBul(seg, "strLeague");
      String sh = degerBul(seg, "intHomeScore"), sa = degerBul(seg, "intAwayScore");
      long yerel = t + 3 * 3600L;
      char sa5[8];
      snprintf(sa5, sizeof(sa5), "%02d:%02d", (int)((yerel % 86400L) / 3600L), (int)((yerel % 3600L) / 60L));
      if (t < now && sh.length() > 0 && sa.length() > 0) {
        if (fark == 0) metin += String("BUGUN BITTI: ") + ev + " " + sh + "-" + sa + " " + dep + " (" + lig + "). ";
      } else if (t >= now - 3 * 3600L && t < now + 36 * 3600L) {
        metin += String(fark == 0 ? "BUGUN " : "YARIN ") + sa5 + ": " + ev + " - " + dep + " (" + lig + "). ";
        if (fark == 0 && t > now) macKur(takim + "|" + sa5 + "|" + bugunTarih());
      }
    }
  }
  return tamam;
}

void gunMaciKontrol() {
  static unsigned long sonDeneme = 0;
  long now = simdiEpoch();
  if (now == 0 || gMisafir || WiFi.status() != WL_CONNECTED || millis() < 90000UL) return;
  struct tm z;
  time_t tt = (time_t)now;
  localtime_r(&tt, &z);
  if (z.tm_hour < 15 || z.tm_hour >= 23) return;
  String g = bugunTarih();
  if (nvsOku("gm_gun") == g) return;
  if (sonDeneme != 0 && millis() - sonDeneme < 15UL * 60000UL) return;
  sonDeneme = millis();
  String takimlar = nvsOku("takimlar");
  if (takimlar.length() == 0) takimlar = "galatasaray";
  String metin = "";
  bool hepsi = true;
  int bas = 0;
  while (bas < (int)takimlar.length()) {
    int e = takimlar.indexOf(',', bas);
    if (e < 0) e = takimlar.length();
    String t = takimlar.substring(bas, e);
    bas = e + 1;
    t.trim();
    if (t.length() == 0) continue;
    if (!takimMaciBul(t, now, metin)) hepsi = false;
  }
  if (!hepsi && metin.length() == 0) return;   // ag/sunucu sorunu: 15 dk sonra tekrar dene
  nvsYaz("gm_gun", g);
  if (metin.length() == 0) return;             // bugun/yarin maci yok, sessiz kal
  if (metin.length() > 350) metin = metin.substring(0, 350);
  bildirBaslik("Mac haberi", metin, 3, "soccer");
  robotKonusur("(Takiminin macini kendiliginden hatirlat)", String("GUNUN MACI: Kullanici bir sey sormadi, takiminin maci var. Kisa (en fazla 2 cumle), dogal ve heyecanli bir yorum yap, saati soyle, bitmis maci varsa skoruna tepki ver. Bilgi (internetten): ") + metin + " Bilgide olmayan bir seyi uydurma.");
}

bool paket29Komut(const String& ad, const String& ar) {
  if (ad == "dolgu") {
    String a = sadelestir(ar);
    nvsYaz("dolgu", (a.indexOf("kapat") >= 0 || a == "0") ? "0" : "1");
  } else if (ad == "takim") {
    int c = ar.indexOf('|');
    String islem = (c > 0) ? sadelestir(ar.substring(0, c)) : String("ekle");
    String t = sadelestir((c > 0) ? ar.substring(c + 1) : ar);
    t.trim();
    if (t.length() == 0) return true;
    String liste = nvsOku("takimlar");
    if (liste.length() == 0) liste = "galatasaray";
    if (islem.indexOf("sil") >= 0) {
      String l2 = String(",") + liste + ",";
      l2.replace(String(",") + t + ",", ",");
      while (l2.startsWith(",")) l2 = l2.substring(1);
      while (l2.endsWith(",")) l2.remove(l2.length() - 1);
      liste = l2;
    } else if ((String(",") + liste + ",").indexOf(String(",") + t + ",") < 0) {
      liste += String(",") + t;
    }
    nvsYaz("takimlar", liste);
    nvsYaz("gm_gun", "");   // hemen yeniden bak
  } else if (ad == "nobet") {
    String a = sadelestir(ar);
    if (a.indexOf("kapat") >= 0) {
      gNobet = false;
    } else {
      gNobet = true;
      gNobetBitis = millis() + 10UL * 3600000UL;
      gNobetEsik = (a.indexOf("hassas") >= 0) ? 6000 : 9000;
      gNobetSay = 0;
      gNobetStt = 0;
      gNobetSon = 0;
      gUyku = true;
    }
  } else return false;
  return true;
}

void paket29Hazirla() {
  geriDonDogrula();
  long now = simdiEpoch();
  if (now == 0) return;
  time_t tt = (time_t)now;
  struct tm z;
  localtime_r(&tt, &z);
  if (z.tm_hour >= 22 || z.tm_hour < 6) { gMood = M_UYKULU; gMoodBitis = millis() + 90000UL; }   // gece gozler uykulu
}
// ===== BUYUK PAKET 29 BITIS =====

void setup() {
  Serial.begin(115200);
  delay(1500);
  acilisKontrol();

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
  if (WiFi.status() != WL_CONNECTED) kurulumAP();
  zamanBaslat();
  alarmYukle();
  webBaslat();
  molaYukle();
  bildir(String("Finduk acildi. Surum ") + FW_SURUM + ". " + resetNedeni());
  kotuSurumBildir();
  Serial.println("Finduk: kablosuz guncelleme calisiyor.");
  guncellemeKontrol();
  girisSelami();
}

void loop() {
  Serial.println();
  Serial.println("Dinliyorum... (konusmaya basla)");
  gDurum = D_BOSTA;
  paket29Hazirla();
  if (gUyku && gNobet) { nobetDongusu(); return; }
  if (gUyku) {
    Serial.println("Uyuyorum... (bir ses cikar, uyanayim)");
    if (!bekleSes(60000UL, ESIK_UYKU)) {
      if (alarmCal()) return;
      fiyatKontrol();
      gunOzetiKontrol();
      ozelGunKontrol();
      hatirlaticiKontrol();
      haftalikAnlarKontrol();
      paket28Periyodik();
      gKonustu = false;
      telefonMesajiIsle();
      if (millis() - sonGuncelKontrol > GUNCEL_ARALIK) guncellemeKontrol();      // uyurken yeni surum var mi diye bak
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
    hikayeBitir();
    fiyatKontrol();
    gunOzetiKontrol();
    ozelGunKontrol();
    hatirlaticiKontrol();
    haftalikAnlarKontrol();
    paket28Periyodik();
    if (gKonustu) { gKonustu = false; return; }
    telefonMesajiIsle();
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

  // Hikaye kayit modu: duyulani oldugu gibi biriktir, "hikaye bitti" deyince telefona gonder
  if (gHikayeBaslik.length() > 0) {
    String hs = sadelestir(duyulan);
    if (hs.indexOf("hikaye bitti") >= 0 || hs.indexOf("kaydi bitir") >= 0 || hs.indexOf("bu kadar") >= 0) {
      String b = gHikayeBaslik;
      hikayeBitir();
      konus(String("Tamam, ") + b + " hikayesini kaydettim, telefonuna gonderdim.");
    } else {
      gHikayeMetin += duyulan + " ";
      if (gHikayeMetin.length() > 3500) {
        String b = gHikayeBaslik;
        hikayeBitir();
        gHikayeBaslik = b + " (devam)";
      }
      konus("Dinliyorum, devam et.");
    }
    sohbetBitis = millis() + SOHBET_SURE;
    gDurum = D_BOSTA;
    return;
  }

  // Adi gecmiyorsa ve sohbet penceresi kapaliysa cevap verme
  if (gKisi.length() && millis() > gKisiBitis) gKisi = "";
  bool adiVar = adiGeciyor(duyulan);
  bool tarifSoz = tarifSozMu(sadelestir(duyulan));
  if (!adiVar && !tarifSoz && millis() > sohbetBitis) {
    gDurum = D_BOSTA;
    Serial.println("(Adimi duymadim, yoksayiyorum. Hey Finduk diye basla.)");
    delay(500);
    return;
  }

  String bilgi = "";
  String sd = sadelestir(duyulan);
  // --- Paket 28: yerel (API'siz) komutlar ---
  if (tarifSoz && tarifKomutu(sd)) { sohbetBitis = millis() + SOHBET_SURE; gDurum = D_BOSTA; return; }
  if (sd.indexOf("ben demet") >= 0 || sd.indexOf("demet konusuyor") >= 0) { gKisi = "demet"; gKisiBitis = millis() + 20UL * 60000UL; if (!gMisafir) emanetTeslim("demet", "Demet"); }
  else if (sd.indexOf("ben oguzhan") >= 0 || sd.indexOf("oguzhan konusuyor") >= 0) { gKisi = ""; if (!gMisafir) emanetTeslim("oguzhan", "Oguzhan"); }
  if (sd.indexOf("misafir") >= 0 && (sd.indexOf("kapat") >= 0 || sd.indexOf("gitti") >= 0 || sd.indexOf("bitti") >= 0)) gMisafir = false;
  else if (sd.indexOf("misafir modu") >= 0 || sd.indexOf("misafir geldi") >= 0 || sd.indexOf("misafirler geldi") >= 0) { gMisafir = true; gMisafirBitis = millis() + 4UL * 3600000UL; }
  if (!gMisafir && sd.indexOf("kasa") >= 0 && (sd.indexOf("kasayi") >= 0 || sd.indexOf("kasam") >= 0 || sd.indexOf("kasa ac") >= 0) && sd.indexOf("ekle") < 0 && sd.indexOf("kaydet") < 0) {
    String kc = kasaAc(sd);
    konus(kc.length() ? kc : String("Parola eslesmedi."));
    sohbetBitis = millis() + SOHBET_SURE;
    gDurum = D_BOSTA;
    return;
  }
  dusunmeSesi();
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
    bool tf = sd.indexOf("havalimani") >= 0 || sd.indexOf("ucus") >= 0 || sd.indexOf("tahmin") >= 0 || sd.indexOf("taf") >= 0;
    bilgi = havaBilgisi();
    String mt = metarBilgisi(tf);
    if (mt.length() > 0) bilgi += String(" ") + mt;
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

  if (!gMisafir) {
    String ek = "";
    if (notMi(sd)) {
      String nl = notlariAl();
      ek += String("Kayitli notlar: ") + (nl.length() ? nl : String("(yok)")) + ". ";
    }
    if (alarmMi(sd)) {
      String al = alarmListesi();
      ek += String("Kurulu alarmlar: ") + (al.length() ? al : String("(yok)")) + ". ";
    }
    if (sd.indexOf("iyi geceler") >= 0 || sd.indexOf("yatiyorum") >= 0 || sd.indexOf("uyuyorum") >= 0) {
      String hf = haftalikHava();
      ek += String("IYI GECELER: Kayitli alarmlar: ") + alarmListesi() + ". Yarinin havasi icin: " + hf + " Yarinin plani: " + planAl() + ". ";
    }
    if (sd.indexOf("fiyat alarm") >= 0 || sd.indexOf("haber ver") >= 0 || sd.indexOf("olunca") >= 0) ek += String("Kayitli fiyat alarmlari: ") + nvsOku("fiyat") + ". ";
    if (aliskanlikMi(sd)) ek += aliskanlikOzet();
    {
      String og = ozelGunler(sd.indexOf("ozel gun") >= 0 || sd.indexOf("dogum gunu") >= 0 || sd.indexOf("yildonum") >= 0 ? 366 : 0);
      if (og.length()) ek += String("OZEL GUNLER: ") + og;
      String em = emanetler();
      if (em.length()) ek += String("BEKLEYEN EMANET MESAJLAR (kime: mesaj): ") + em + " ";
      if (sd.indexOf("hatirlat") >= 0) ek += String("Kayitli tekrarlayan hatirlaticilar: ") + nvsOku("hatirlat") + " ";
      if (sd.indexOf("gunluk") >= 0 || sd.indexOf("guzel an") >= 0 || sd.indexOf("bu hafta") >= 0) ek += String("Bu haftanin guzel anlari: ") + haftaninAnlari() + " ";
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
    if (sd.indexOf("vardiya") >= 0 || sd.indexOf("nobet") >= 0 || sd.indexOf("uyku duzeni") >= 0) ek += vardiyaOzet() + " ";
    if (sd.indexOf("muayene") >= 0 || sd.indexOf("sigorta") >= 0 || sd.indexOf("kasko") >= 0 || sd.indexOf("arac bakim") >= 0 || sd.indexOf("araba") >= 0 || sd.indexOf("yag degis") >= 0 || sd.indexOf("lastik") >= 0) ek += aracOzet() + " ";
    if (sd.indexOf("yemek") >= 0 || sd.indexOf("ne pisir") >= 0 || sd.indexOf("ne yesek") >= 0 || sd.indexOf("menu") >= 0 || sd.indexOf("kahvalti") >= 0) ek += String("Kayitli yemek plani: ") + satirlariAl("yemekplan") + " ";
    if (sd.indexOf("hediye") >= 0 || sd.indexOf("surpriz") >= 0 || sd.indexOf("yildonum") >= 0 || sd.indexOf("dogum gunu") >= 0 || sd.indexOf("evlilik") >= 0) ek += String("Kayitli ozel gunler: ") + ozelGunler(366) + " ";
    if (sd.indexOf("tekerleme") >= 0 || sd.indexOf("siir") >= 0 || sd.indexOf("gunun sarki") >= 0) { String ip = ""; for (int i = 0; i < hN; i++) ip += hSoru[i] + ". "; ek += String("GUNUN TEKERLEMESI icin bugunku konusma ipuclari: ") + ip + " Guzel anlar: " + haftaninAnlari() + " "; }
    if (sd.indexOf("gunlugun") >= 0) ek += String("Finduk'un kendi gunlugu (son girisler): ") + satirlariAl("rgunluk") + " ";
    if (sd.indexOf("bir yil once") >= 0 || sd.indexOf("gecmis yil") >= 0) ek += String("Gecmis yillarin bugunku anilari: ") + yilOnceAni() + " ";
    { String ca = cihazAdlari(); if (ca.length()) ek += String("Kayitli ev cihazlari (cihaz komutuyla acilip kapanir): ") + ca + " "; }
    if (nvsOku("tvip").length()) ek += "Televizyon tanimli, tv komutuyla kontrol edebilirsin. ";
    if (ek.length() > 0) bilgi = (bilgi.length() > 0) ? bilgi + " " + ek : ek;
  }

  { String p29 = paket29Bilgi(sd); if (p29.length()) bilgi = bilgi.length() ? bilgi + " " + p29 : p29; }
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
    nvsYaz("son_gorus", String(simdiEpoch()));
    gSonEtkilesim = millis();
    sayacArttir("say_k");
    if (gKisi.length()) gKisiBitis = millis() + 20UL * 60000UL;
    gMood = mood;
    konus(cevap);
    for (int i = 0; i < komutN; i++) {      // konusma bitince uygulamaya komutlari yolla
      if (gMisafir && !komutlar[i].startsWith("goz") && !komutlar[i].startsWith("misafir")) continue;
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
