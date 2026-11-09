# Sinema Odası günlüğü

## 2026-11-09
- Yeni komutlar: `yakindasec|N` (Yakında ekranındaki N. filmi açar), `ajandaac|N` (ajandanın N. satırını açar), `sahnesec|Ankara` ya da `sahnesec|konser` (Sahne ekranında şehir/tür seçer).
- Durum bilgisi genişledi: Yakında ekranında vizyon listesi ve ajanda, Sahne ekranında seçili şehir/tür ve biletçi adları Finduk'a bildirilir.
- Robot tarafında yalnızca KISILIK komut listesi ve sinemaMi anahtar kelimeleri (vizyon, sahne, ajanda, yakinda) güncellendi.
- Nasıl denenir: "Finduk, yakında vizyona girecek ilk filmi aç" ya da "Sahne'de Ankara'yı seç". Sahte DOM (jsdom) ile denendi; gerçek telefonda denenmedi.
