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
