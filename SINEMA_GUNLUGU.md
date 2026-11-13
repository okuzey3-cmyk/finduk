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
