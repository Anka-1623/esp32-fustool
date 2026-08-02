# FUSTOOL-ESP32

**FUSTOOL'un ESP32 icin yazilan surumu** — ESP32-S3 (N16R8 ve benzeri) genel amacli development kartlari uzerinde calisan bir WiFi/Bluetooth guvenlik test araci. Ekran, SD kart veya butona ihtiyac duymadan, kartin actigi kendi WiFi agina baglanip tarayicidan **FUSTOOL Control** paneli uzerinden kullanilir.

Bu proje [justcallmekoko/ESP32Marauder](https://github.com/justcallmekoko/ESP32Marauder) tabanlidir; orijinal projeye ve yazarina tesekkurler. Lisans (GPL-3.0) ve atif korunmustur, bkz. [LICENSE](LICENSE).

## Neler farkli?

- **Genel ESP32-S3 destegi**: Orijinal proje klasik ESP32 ve belirli hazir Marauder kartlarini hedefler; bu fork `GENERIC_ESP32` profiliyle ekransiz/SD'siz herhangi bir ESP32-S3 devkite (N16R8 dahil, 8MB octal PSRAM aktif) kurulacak sekilde yapilandirilmistir.
- **WebUI paneli** (`esp32_marauder/WebUI.h/.cpp`): Kart kendi WiFi erisim noktasini (AP) acar, taraayicidan baglanip:
  - WiFi'daki yakin agları (AP baglantisini dusurmeden) listeler,
  - Bluetooth (BLE) cihazlarini canli listeler,
  - Bulunan bir aga tiklayinca **veri topla / saldiri** aksiyonlarini (sinirli sureli, otomatik durup baglantiyi geri getiren) sunar,
  - Marauder'in tum orijinal komut satiri komutlarini bir konsol kutusundan calistirir,
  - Yakalanan `.pcap`/handshake dosyalarini (dahili flash / FFat) listeler ve indirir.
- **Dahili flash depolama**: SD kart olmadan da paket yakalama (pcap/log) dosyalari `FFat` (16MB flash uzerindeki FAT bolumu) icine yazilir.

## Kurulum (ozet)

1. `arduino-cli` + `esp32:esp32@2.0.11` board core kurulu olmali.
2. Bagimliliklari `esp32_marauder/` yaninda `libraries/` altina kurun (bkz. `.github/workflows` kaldirildigindan surumler icin orijinal ESP32Marauder repo CI dosyasina bakabilirsiniz: TFT_eSPI, NimBLE-Arduino, ESPAsyncWebServer, AsyncTCP, ArduinoJson, LinkedList, vb.).
3. Derleme/flash:
   ```
   arduino-cli compile --fqbn "esp32:esp32:esp32s3:PartitionScheme=app3M_fat9M_16MB,FlashSize=16M,PSRAM=opi" \
     --libraries ~/Arduino/libraries esp32_marauder
   arduino-cli upload --fqbn "esp32:esp32:esp32s3:PartitionScheme=app3M_fat9M_16MB,FlashSize=16M,PSRAM=opi" \
     -p /dev/ttyACM0 esp32_marauder
   ```
4. Kart acilinca seri konsolda AP adi/sifresi ve panel adresi (`http://192.168.4.1:8080/`) yazdirilir. AP adini/sifresini degistirmek icin `esp32_marauder/WebUI.h` basindaki `WEBUI_AP_SSID` / `WEBUI_AP_PASS` degerlerini duzenleyip yeniden flashlayin.

## Sorumlu kullanim

Bu, gercek WiFi/Bluetooth guvenlik testi yapabilen (deauth, paket yakalama, tarama) bir araçtir. Sadece **kendi ağınızda** veya **yazılı izniniz olan** ortamlarda kullanın; başkasının ağında izinsiz kullanmak yasa dışıdır.
