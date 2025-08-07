Teks Berjalan dan Jam pada Panel P10 dengan Web Server dan RTC
Proyek ini adalah sistem berbasis ESP8266 untuk mengendalikan panel LED P10, yang menampilkan teks berjalan dan jam secara bergantian. Sistem ini dilengkapi dengan antarmuka web untuk mengatur teks, kecepatan scroll, kecerahan, dan waktu, serta menyimpan pengaturan ke EEPROM. Modul RTC DS3231 digunakan untuk menjaga akurasi waktu.
Fitur

Menampilkan teks berjalan dan jam secara bergantian setiap 30 detik.
Antarmuka web untuk mengatur:
Teks yang ditampilkan (maksimal 49 karakter).
Kecepatan scroll (10-200 ms).
Kecerahan display (0-255).
Pengaturan waktu RTC.


WiFi Access Point (AP) dengan captive portal untuk akses mudah.
Penyimpanan pengaturan ke EEPROM untuk mempertahankan data setelah restart.
Status RTC dan waktu ditampilkan di antarmuka web.

Dependensi
Perpustakaan yang diperlukan:

DMDESP - Untuk mengendalikan panel P10.
RTClib - Untuk komunikasi dengan modul RTC DS3231.
ESP8266WiFi - Untuk fungsi WiFi.
ESP8266WebServer - Untuk server web.
EEPROM - Untuk penyimpanan pengaturan.
DNSServer - Untuk captive portal.

Instal perpustakaan ini melalui Library Manager di Arduino IDE.
Diagram Wiring
Berikut adalah koneksi pin antara ESP8266 (NodeMCU), panel P10, dan modul RTC DS3231:



Perangkat
Pin ESP8266 (NodeMCU)
Pin Panel P10 / RTC



Panel P10




A
D0 (GPIO16)
A


B
D1 (GPIO5)
B


CLK
D5 (GPIO14)
CLK


SCK
D3 (GPIO0)
SCK


R
D2 (GPIO4)
R


EN
D4 (GPIO2)
EN


GND
GND
GND


VCC
5V (atau sumber eksternal)
VCC


RTC DS3231




SDA
D6 (GPIO12)
SDA


SCL
D7 (GPIO13)
SCL


VCC
3.3V
VCC


GND
GND
GND


Catatan:

Panel P10 membutuhkan daya 5V. Jika menggunakan beberapa panel, gunakan sumber daya eksternal dengan arus yang cukup (biasanya 4A atau lebih per panel).
Pastikan koneksi I2C (SDA, SCL) untuk RTC menggunakan pull-up resistor (biasanya sudah ada di modul DS3231).
Hubungkan GND dari semua perangkat ke ground yang sama.

Konfigurasi WiFi

SSID: P10_Display
Password: 12345678
IP Address: 192.168.4.1 (default untuk Access Point)

Setelah ESP8266 dinyalakan, sambungkan perangkat Anda ke WiFi P10_Display. Browser akan otomatis mengarahkan ke halaman konfigurasi (captive portal) di http://192.168.4.1.
Cara Instalasi

Persiapan Perangkat Keras:

Hubungkan panel P10 dan modul RTC DS3231 ke ESP8266 sesuai diagram wiring di atas.
Pastikan sumber daya mencukupi untuk panel P10.


Persiapan Perangkat Lunak:

Buka Arduino IDE.
Instal perpustakaan yang diperlukan melalui Library Manager.
Salin file P10_Display.ino dan P10_Display.h ke folder proyek yang sama.


Upload Kode:

Hubungkan ESP8266 ke komputer via USB.
Pilih board NodeMCU 1.0 (ESP-12E Module) di Arduino IDE.
Upload kode ke ESP8266.


Akses Antarmuka Web:

Sambungkan ke WiFi P10_Display dengan password 12345678.
Buka browser dan akses http://192.168.4.1.
Gunakan antarmuka web untuk mengatur teks, kecepatan scroll, kecerahan, dan waktu.



Penggunaan

Antarmuka Web:

Teks: Masukkan teks baru (maks. 49 karakter) untuk ditampilkan sebagai teks berjalan.
Kecepatan Scroll: Atur kecepatan scroll dengan slider (10 = cepat, 200 = lambat).
Kecerahan: Atur kecerahan display dengan slider (0 = redup, 255 = terang).
Waktu: Atur waktu RTC melalui input waktu.
Reset RTC: Jika RTC error, tombol reset akan muncul untuk mengatur ulang ke 12:00.
Simpan: Tekan tombol "Save All Settings" untuk menyimpan perubahan ke EEPROM.
Refresh: Tekan tombol "Refresh Status" untuk memperbarui status.


Tampilan:

Panel P10 akan bergantian menampilkan teks berjalan dan jam setiap 30 detik.
Jika RTC error, panel akan menampilkan "ERROR".



Catatan

Pastikan modul RTC DS3231 memiliki baterai cadangan untuk menjaga waktu saat daya mati.
Jika WiFi tidak terdeteksi, periksa kembali koneksi dan pastikan ESP8266 berfungsi dengan baik.
Untuk debugging, buka Serial Monitor (115200 baud) untuk melihat status RTC dan log lainnya.

Lisensi
Proyek ini open-source dan dapat digunakan serta dimodifikasi sesuai kebutuhan. Tidak ada jaminan yang diberikan, gunakan dengan tanggung jawab sendiri.
