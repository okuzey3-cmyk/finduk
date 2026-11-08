# Finduk
ESP32-S3 sesli asistan robot (INMP441 mikrofon, MAX98357A hoparlor, OLED gozler).
Ses -> Gemini (yaziya) -> Claude (cevap) -> Gemini (ses).

- `sketch/finduk.ino`: Arduino kodu. Wi-Fi sifresi ve API anahtarlari `BURAYA_YAZ` olarak birakildi, kendi anahtarlarini yaz. Anahtarlari GitHub'a yukleme.
- `stl/`: 3D baski parcalari (kafa, on plaka, boyun, govde 5W, taban 5W).
- `cad/`: parcalari ureten Python kodlari.

Arduino ayari: Board ESP32S3 Dev Module, PSRAM: OPI PSRAM.
