# Sinema Odası günlüğü

## 2026-11-09
- Yeni komutlar: `yakindasec|N` (Yakında ekranındaki N. filmi açar), `ajandaac|N` (ajandanın N. satırını açar), `sahnesec|Ankara` ya da `sahnesec|konser` (Sahne ekranında şehir/tür seçer).
- Durum bilgisi genişledi: Yakında ekranında vizyon listesi ve ajanda, Sahne ekranında seçili şehir/tür ve biletçi adları Finduk'a bildirilir.
- Robot tarafında yalnızca KISILIK komut listesi ve sinemaMi anahtar kelimeleri (vizyon, sahne, ajanda, yakinda) güncellendi.
- Nasıl denenir: "Finduk, yakında vizyona girecek ilk filmi aç" ya da "Sahne'de Ankara'yı seç". Sahte DOM (jsdom) ile denendi; gerçek telefonda denenmedi.

## 2026-11-10
- Rafım ekranı genişledi: `rafsec|N` (raftaki N. filmi açar), `rafara|metin` (rafta arar, boş metin aramayı temizler), `rafsirala|yeni/puan/yil/ad`, `rafmenu|biz/atolye/pasaport/gunluk/yil/gorev/rozet` (Rafım'daki bölümleri açar; sadece görüntüleme).
- Durum bilgisi: açık raf sekmesi, sıralama, üç rafın film sayıları ve raf araması Finduk'a bildirilir.
- Hata düzeltme: sinemaMi'de dizi 20 anahtar kelimeydi ama döngü 16'ya kadardı (vizyon, sahne, ajanda, yakinda hiç denenmiyordu); dizi 24 kelimeye çıkarıldı (pasaport, rozet, gunluk, gorev eklendi), döngü 24.
- Nasıl denenir: "Finduk, rafımı puana göre sırala" ya da "Finduk, rozetlerimi aç". Sahte DOM (jsdom) ile denendi; gerçek telefonda ve robotta denenmedi.

## 2026-11-11
- Yeni komutlar: `neizlesem|gul;kisa;cift` ("3 soruda film seç" penceresini açıp ruh hali, süre, kiminle sorularını cevaplar; eksik yazılırsa o adımda durur), `nebaska` (aynı seçimle başka 6 film), `nebastan`, `ovsec|N` (açık penceredeki N. filmi açar), `filmgecesi` (açık biletin film gecesi menüsü), `kadifesor` (açık yapımı Kadife'ye sorar).
- Durum bilgisi: açık pencerenin adımı, seçenekleri, önerilen filmler ve film gecesi menüsü Finduk'a bildirilir. `kapat` artık açık pencereyi de kapatır.
- Nasıl denenir: "Finduk, gülmek istiyorum, kısa bir film, ikimiz izleyeceğiz". Sahte DOM (jsdom) ile denendi; gerçek telefonda ve robotta denenmedi.

## 2026-11-12
- Yeni komutlar: `eslestir|ayni film` (birlikte film eşleştirme penceresini aynı telefon modunda açar; `eslestir|dizi` dizi seçer, sadece `eslestir` mod seçimine kadar açar), `begen` / `gec` (eşleştirme kartını beğenir ya da geçer), `oyla|izleyecek` ya da `oyla|kadife` (3 adaylı oylama penceresini açar, adaylar `ovsec|N` ile işaretlenir), `kadifeses|ac` / `kadifeses|kapat` (Kadife'nin sesli yanıtı).
- Güvenlik: WhatsApp ile gönderen düğmeler `dugme` komutuyla basılamasın diye YASAK listesine whatsapp, gönder, bildir, paylaş, yedekle eklendi; oylama/eşleştirme gönderimini kullanıcı kendisi yapar.
- Durum bilgisi: eşleştirme kartında görünen yapım ve oylamada seçilen adaylar bildirilir. sinemaMi dizisi 28 kelime (esles, oylama, kartta, secilen eklendi), döngü 28.
- Nasıl denenir: "Finduk, Demet'le film eşleştirelim, aynı telefondayız". Sahte DOM (jsdom) ile denendi (aday yüklenmesi ağ olmadığı için denenemedi); gerçek telefonda ve robotta denenmedi.

## 2026-11-13
- Yeni komutlar (Keşfet ekranı): `salon|netflix,hbo` (Salonlarım'da sadece yazılan platformlar açık kalır), `salon|hepsi`, `salon|+hbo` / `salon|-prime` (tek platformu açar/kapatır), `salonyeni|ac` / `salonyeni|kapat` (sadece son 1 yılın yapımları), `paketsec|N` (Paketinde yeni çıkanlar şeridindeki N. yapım), `zevksec|N` (Senin zevkine göre şeridindeki N. yapım), `kadifeayar` (Kadife ayarlarını açar, sadece gösterir).
- Durum bilgisi: açık/kapalı salonlar, Yeni modu, iki şeridin yapım adları ve Kadife ayarları açıksa Kadife'nin zevk notları Finduk'a bildirilir.
- Robot tarafı: yalnızca KISILIK komut cümlesi ve sinemaMi (28 → 32 kelime: salon, paketim, zevk, platform; döngü 32).
- Nasıl denenir: "Finduk, sadece Netflix ve HBO açık kalsın" ya da "Senin zevkine göre ilk filmi aç". Sahte DOM (jsdom) ile salon komutları denendi (bu sırada "salon" sınıf adındaki "on" harflerinden doğan bir hata bulunup düzeltildi); gerçek telefonda ve robotta denenmedi.

## 2026-11-14
- Yeni komutlar: `benzer|N` (açık biletteki "Buna benzeyenler" şeridinin N. yapımını açar), `seri|N` (açık filmin serisindeki N. filmi açar; zaten açık olanı atlar).
- Durum bilgisi: açık biletin benzer yapımları ve serinin filmleri (şu an açık olan işaretli) Finduk'a bildirilir.
- Robot tarafı: yalnızca KISILIK komut cümlesi ve sinemaMi (32 → 34 kelime: benzer, seri; döngü 34). Süslü parantez farkı değişmedi (6).
- Nasıl denenir: "Finduk, buna benzeyen ilk filmi aç" ya da "Serinin ikinci filmini aç". Sahte DOM (jsdom) ile denendi; gerçek telefonda ve robotta denenmedi.

## 2026-11-15 — BÜYÜK GÜNCELLEME
- Yeni komutlar: `gecehazirla|gul;kisa;cift` (çok adımlı: "3 soruda film seç"i cevaplar, ilk önerilen filmi açar, film gecesi menüsünü getirir; `;2` ile ikinci öneri, sona `;fragman` ile fragman da başlar), `rafac|favori;2` / `rafac|izlenecek;Inception` / `rafac|izlenen;1` (raf sekmesini açıp sıraya ya da film adına göre filmi açar).
- Durum bilgisi: yeni "Alışkanlıkların" bölümü (en çok izlenen/favori türler, puan ortalaması, son izlenenler, 5 yıldızlılar, son favoriler, izlenecek sayısı) ve açık biletteki senin puanın/notun; durum metni sınırı 1800 → 2300 karakter.
- Hata düzeltme: `puanla|8` gibi 1-10 puanlar 5 yıldıza çevrilir (önceden 5'ten büyük puan sessizce işe yaramıyordu).
- Robot tarafı: yalnızca KISILIK komut cümleleri (sinemaMi aynı, 34 kelime). Süslü parantez farkı değişmedi (6).
- Nasıl denenir: "Finduk, film gecesi hazırla, gülmek istiyoruz, kısa, ikimiz" ya da "Finduk, favorilerimden ikincisini aç" / "Ne tür filmleri seviyorum?". Sahte DOM (jsdom) ile rafac ve alışkanlık bilgisi denendi; gecehazirla'nın sonuç aşaması (ağ gerektiriyor) ve gerçek telefon/robot denenmedi.

## 2026-11-16
- Yeni komutlar (Kadife): `kadifeoner|N` (Kadife'nin sohbetteki önerdiği N. devam çipine basar; Kadife yazıyorsa basmaz), `kadifehava|esprili/ciddi/uzun/samimi` (Kadife'nin konuşma havasını ayarlar; sadece bu ayarı yazar, Gemini anahtarına ve verilere dokunmaz).
- Durum bilgisi: Kadife ekranında Kadife'nin havası, sesinin açık/kapalı olduğu, yazıp yazmadığı ve önerdiği devam çipleri Finduk'a bildirilir.
- Robot tarafı: yalnızca KISILIK komut cümlesi (sinemaMi aynı, 34 kelime). Süslü parantez farkı değişmedi (6).
- Nasıl denenir: "Finduk, Kadife daha esprili olsun" ya da "Kadife'nin ilk önerisine bas". Sahte DOM (jsdom) ile hava komutu ve durum metni denendi; çip tıklaması jsdom'da Kadife meşgul göründüğü için denenemedi; gerçek telefon/robot denenmedi.

## 2026-11-17
- Yeni komut (Sahne): `biletci|N` ya da `biletci|Biletix` (Sahne ekranındaki N. biletçiyi / adını içeren biletçiyi açar; telefon tarayıcısı açılır pencereyi engellerse açılmayabilir). `sahnesec` artık tek seferde birden fazla seçim alır: `sahnesec|konser;Ankara`.
- Durum bilgisi: Sahne'deki biletçiler artık numaralı bildirilir (1) Bubilet ...).
- Robot tarafı: yalnızca KISILIK komut cümlesi (sinemaMi aynı, 34 kelime; "biletci" zaten "bilet" ile yakalanır). Süslü parantez farkı değişmedi (6).
- Nasıl denenir: "Finduk, Ankara'da konserlere bak" ya da "İkinci biletçiyi aç". Sözdizimi node ile denendi; jsdom ile denenmedi, gerçek telefon/robot denenmedi.

## 2026-11-18
- Yeni komutlar: `oyuncu|2` / `oyuncu|Ayşe` / `oyuncu|yonetmen` (açık biletteki oyuncunun ya da yönetmenin kişi kartını açar), `kisifilm|N` (açık kişi kartındaki N. yapımı açar).
- Durum bilgisi: oyuncular artık numaralı ve rolleriyle, yönetmen ve açık kişi kartı (ad, kısa biyografi, kişinin işleri) Finduk'a bildirilir.
- Robot tarafı: yalnızca KISILIK komut cümlesi (sinemaMi aynı, 34 kelime; "oyuncu" zaten var). Süslü parantez farkı değişmedi (6).
- Nasıl denenir: "Finduk, ikinci oyuncuyu aç" sonra "Onun üçüncü filmini aç". Sahte DOM (jsdom) ile oyuncu/yönetmen/kisifilm ve kişi kartı durumu denendi; açık biletteki oyuncu durum satırı ve gerçek telefon/robot denenmedi.

## 2026-11-19
- Yeni komut: `gorevkadife` (açık "Bu ayın görevleri" penceresinde Kadife'den yapım önerisi ister). Hata düzeltme: `kapat` artık Rozetlerim penceresini de kapatıyor (önceden sadece ana ekrana dönüp pencere açık kalıyordu).
- Durum bilgisi: Rafım ekranında bu ayın görevleri (n/hedef) ve kazanılan rozet sayısı/adları; görev ya da rozet penceresi açıkken ayrıntılı ilerleme ve sıradaki rozetler Finduk'a bildirilir.
- Robot tarafı: yalnızca KISILIK komut cümlesi (sinemaMi aynı, 34 kelime; "gorev" ve "rozet" zaten var). Süslü parantez farkı değişmedi (6).
- Nasıl denenir: "Finduk, bu ayın görevlerini aç, sonra Kadife'den öneri iste" ya da "Rozetlerimi aç, kaç tane kazandım?". Sahte DOM (jsdom) ile görev/rozet açma, durum metni ve kapat denendi; gerçek telefon/robot denenmedi.

## 2026-11-20
- Yeni komutlar: `durak|N` / `durak|Fransa` (açık Sinema Pasaportum penceresinde "Sıradaki duraklar"dan birine basar; çıkan filmler `ovsec|N` ile açılır), `gunlukgun|7` (Sinema Günlüğüm takviminde o günün izlenenlerini açar), `gunlukay|onceki` / `gunlukay|sonraki` (takvimde ay değiştirir).
- Durum bilgisi: Pasaport damgaları (ülke, yapım sayısı), sıradaki duraklar ve günlük ayı/izlenen günler (yapım sayısıyla) Finduk'a bildirilir.
- Robot tarafı: yalnızca KISILIK komut cümlesi (sinemaMi aynı, 34 kelime; "pasaport" ve "gunluk" zaten var). Süslü parantez farkı değişmedi (6).
- Nasıl denenir: "Finduk, sinema pasaportumu aç, sonra Japonya durağına bas" ya da "Günlüğümü aç, geçen aya git". Sahte DOM (jsdom) ile üç komut ve durum metni denendi; gerçek telefon/robot denenmedi.

## 2026-11-21
- Yeni komut: `duello|2` / `duello|Inception` (açık BİZ penceresindeki Puan düellosundan N. filmi ya da adını içeren filmi açar; BİZ penceresi `rafmenu|biz` ile açılır).
- Durum bilgisi: BİZ ekranında zevk uyumu yüzdesi, puan düellosu satırları (numaralı) ve özel günler Finduk'a bildirilir.
- Robot tarafı: yalnızca KISILIK komut cümlesi (sinemaMi aynı, 34 kelime; "biz" ve "rafmenu" zaten var). Süslü parantez farkı değişmedi (6).
- Nasıl denenir: "Finduk, Biz ekranını aç, uyumumuz kaç?" sonra "İlk düelloyu aç". Yalnızca sözdizimi node ile denendi; jsdom yok, gerçek telefon/robot denenmedi (BİZ ekranı için eşin rafının telefonda olması gerekir).

## 2026-11-22 — BÜYÜK GÜNCELLEME (Sinema Atölyesi + Yıl Hikâyem)
- Yeni komutlar: `atolyeac|Inception;atmosfer` (tek seferde Rafım'dan Sinema Atölyesi'ni açar, filmi seçer, istenen bölümü açar: atmosfer, muzik, yonetmen, harita, duello), `atolyefilm|2` / `atolyefilm|Amelie` (atölyede film seçer), `atolyemod|harita` (seçili filmle bölüm açar; `atolyemod|degistir` filmi değiştirir), `hikaye` (Yıl Hikâyem'i açar), `hikaye|ileri` / `hikaye|geri` / `hikaye|3` (sayfa gezer). `kapat` artık Yıl Hikâyem'i de kapatıyor.
- Durum bilgisi: atölyede seçilebilir filmler (numaralı), seçili film ve bölümler; Atmosfer durumu; Yıl Hikâyem'in hangi sayfada olduğu ve sayfa metni; alışkanlıklara "bu yıl N yapım izledin (film/dizi)" eklendi (sınır 520 → 620).
- Robot tarafı: yalnızca KISILIK komut cümlesi ve sinemaMi'ye "atolye", "yil hikaye" kelimeleri (34 → 36, döngü sınırı da 36). Süslü parantez farkı değişmedi (6).
- Nasıl denenir: "Finduk, Amelie ile atölyede atmosferi aç" ya da "Yıl hikâyemi aç, sonraki sayfa". Gerçek Chromium + sahte raf verisiyle atolyeac, atolyefilm, atolyemod, hikaye ve kapat denendi (internet yoktu: harita/müzik gerçek yüklemeleri ve gerçek telefon/robot denenmedi). Önerilen: "Seansa ekle" (takvim dosyası indirir) bilerek eklenmedi.

## 2026-11-23
- Yeni komut: `filtreac` (Keşfet'te Filtre penceresini açar, uygulamaz; `kapat` vazgeçer). `filtre|...` sağlamlaştı: "90'lar" gibi kesme işaretli seçenekler `yil=90lar` ya da `yil=90` ile bulunur, `ne=dizi` de kategori olarak çalışır, boş değer yok sayılır.
- Durum bilgisi: Filtre penceresi açıkken her filtrenin (ne izleyelim, tür, oyuncu, süre, kimle, platform, yıl, puan) şu anki değeri ve seçenekleri Finduk'a bildirilir.
- Robot tarafı: yalnızca KISILIK komut cümlesi (sinemaMi aynı, 36 kelime; "filtre" zaten var). Süslü parantez farkı değişmedi (6).
- Nasıl denenir: "Finduk, filtreleri aç; 90'lardan komedi dizisi olsun". Gerçek Chromium + sahte sayfa ile filtreac, durum metni, filtre ve kapat denendi (internet yoktu); gerçek telefon/robot denenmedi.

## 2026-11-24
- Yeni komut: `sesefekt|ac` / `sesefekt|kapat` (Kadife ayarlarındaki "ses efektleri ve titreşim sesi" ayarını açar/kapatır; Kadife'nin konuşma sesi `kadifeses` ile ayrı kalır).
- Durum bilgisi: Kadife ekranında ses efektlerinin açık/kapalı olduğu da Finduk'a bildirilir.
- Robot tarafı: yalnızca KISILIK komut cümlesi (sinemaMi aynı, 36 kelime). Süslü parantez farkı değişmedi.
- Nasıl denenir: "Finduk, uygulamanın ses efektlerini kapat". Sözdizimi node ile ve komut mantığı ayrıca denendi; jsdom/gerçek telefon/robot denenmedi.

## 2026-11-25
- Yeni komut: `yorum` (açık biletteki izleyici yorumlarını gösterir), `yorum|2` (2. yorumu genişletir). Durumda yorum sayısı, komuttan sonra ilk 3 yorumun metni (ya da seçilen yorum) Finduk'a bildirilir; köşeli parantezler temizlenir.
- Robot tarafı: yalnızca KISILIK komut cümlesi ve sinemaMi (36 → 38 kelime: yorum, ne demis; döngü 38).
- Nasıl denenir: bir film açıkken "Finduk, bu filme ne demişler?" (yorumlar birkaç saniye sonra gelir; gerekirse bir kez daha sor). Gerçek Chromium + sahte yorumlarla denendi (internet yoktu: gerçek yorum/çeviri ve gerçek telefon/robot denenmedi).

## 2026-11-26
- Yeni komut: `takip` (açık kişi kartındaki oyuncu/yönetmeni takibe alır; takipten çıkarmaz, liste 20 kişiyle doluysa eskisi silinmesin diye eklemez). `dugme` ile takip düğmesine basılamaz (YASAK listesine "takip" eklendi).
- Durum bilgisi: kapalı sürpriz zarfın tür/yıl/puan bilgisi ve ipucu (film adı gizli), kişinin takipte olup olmadığı ve takip edilenler (kişi kartında ve Yakında ekranında). Zarf açıkken eski aday listesi bildirilmez.
- Robot tarafı: yalnızca KISILIK komut cümlesi ve sinemaMi (38 → 40 kelime: takip, ipucu; döngü 40). Süslü parantez farkı değişmedi (6).
- Nasıl denenir: "Finduk, sürpriz zarf aç, ipucu ne?" ya da bir film açıkken "Birinci oyuncuyu aç, onu takibe al". Gerçek Chromium + sahte verilerle denendi (internet yoktu); gerçek telefon/robot denenmedi.
