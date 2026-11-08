# Finduk
ESP32-S3 sesli asistan robot (INMP441 mikrofon, MAX98357A hoparlor, OLED gozler).
Ses -> Gemini (yaziya) -> Claude (cevap) -> Gemini (ses). Hava durumu ve haber de soyleyebilir.

- `sketch/finduk.ino`: Arduino kodu. Wi-Fi sifresi ve API anahtarlari kodda YOK; robotun hafizasina seri ekrandan bir kere yazilir.
- `stl/`: 3D baski parcalari. `cad/`: onlari ureten Python kodlari.
- `.github/workflows/build.yml`: `sketch/` degisince GitHub kodu derler, `firmware` dalina `firmware.bin` ve `version.txt` koyar. Robot buradan kendini gunceller.

## Arduino ayarlari (USB ile ilk yukleme)
Board: ESP32S3 Dev Module, PSRAM: OPI PSRAM, Flash Size: 16MB, Partition Scheme: 16M Flash (3MB APP/9.9MB FATFS).

## Anahtar girme (Serial Monitor, 115200, New Line)
`wifi=AGADI`, `sifre=SIFRE`, `gemini=ANAHTAR`, `claude=ANAHTAR`, istege bagli `token=GITHUB_TOKEN` (repo gizliyse), `goster`.
