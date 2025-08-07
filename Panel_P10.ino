
/*
 * Teks Berjalan dan Jam untuk Panel P10 dengan Web Server dan RTC
 * Versi dengan kontrol kecerahan dan kecepatan teks
*/

#include <DMDESP.h>
#include <fonts/ElektronMart6x12.h>
#include <RTClib.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <DNSServer.h>
DNSServer dnsServer;
const byte DNS_PORT = 53;

// SETUP DMD
#define DISPLAYS_WIDE 1
#define DISPLAYS_HIGH 1
DMDESP Disp(DISPLAYS_WIDE, DISPLAYS_HIGH);

// SETUP RTC
RTC_DS3231 rtc;

// SETUP WiFi AP
const char *ssid = "P10_Display";
const char *password = "12345678";
ESP8266WebServer server(80);

// SETUP Teks dan Waktu
char teks[50] = "TES TEXT BERJALAN";
int displayBrightness = 20; // Default brightness (0-255)
int scrollSpeed = 50;       // Default speed (ms)
const int TRANSITION_INTERVAL = 30000;
const int TIME_UPDATE_INTERVAL = 1000;
enum DisplayMode { TEKS, JAM };
DisplayMode currentMode = TEKS;
unsigned long lastTransition = 0;
unsigned long lastTimeUpdate = 0;

// Status RTC
bool rtcValid = false;

// Alamat EEPROM untuk penyimpanan
#define EEPROM_TEXT_START 0
#define EEPROM_BRIGHTNESS 50
#define EEPROM_SPEED 51

//----------------------------------------------------------------------
// SETUP
void setup() {
  Disp.start();
  Disp.setFont(ElektronMart6x12);

  Serial.begin(115200);
  delay(1000);

  EEPROM.begin(512);
  readSettingsFromEEPROM();
  Disp.setBrightness(displayBrightness);

  // Inisialisasi RTC (tetap sama)
  Serial.println("Menginisialisasi RTC...");
  if (!rtc.begin()) {
    Serial.println("ERROR: RTC tidak ditemukan!");
    rtcValid = false;
  } else {
    Serial.println("RTC ditemukan!");
    if (rtc.lostPower()) {
      Serial.println("RTC kehilangan daya, mengatur waktu default...");
      rtc.adjust(DateTime(2024, 1, 1, 12, 0, 0));
    }
    DateTime testTime = rtc.now();
    if (isValidTime(testTime)) {
      rtcValid = true;
      Serial.printf("RTC OK - Waktu: %02d:%02d:%02d\n", 
                   testTime.hour(), testTime.minute(), testTime.second());
    } else {
      Serial.println("ERROR: RTC memberikan waktu tidak valid!");
      rtcValid = false;
      rtc.adjust(DateTime(2024, 1, 1, 12, 0, 0));
      delay(100);
      DateTime retestTime = rtc.now();
      if (isValidTime(retestTime)) {
        rtcValid = true;
        Serial.println("RTC berhasil di-reset!");
      }
    }
  }

  // Setup WiFi AP
  WiFi.softAP(ssid, password);
  Serial.println("WiFi AP: " + String(ssid));
  Serial.println("IP Address: " + WiFi.softAPIP().toString());

  // Setup DNS Server untuk Captive Portal
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  // Setup Web Server
  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.onNotFound(handleNotFound); // Tangani semua permintaan yang tidak dikenali
  server.begin();
  Serial.println("Web server started");
}

//----------------------------------------------------------------------
// LOOP

void loop() {
  Disp.loop();
  server.handleClient();
dnsServer.processNextRequest();
  unsigned long currentMillis = millis();
  
  if (currentMillis - lastTransition >= TRANSITION_INTERVAL) {
    lastTransition = currentMillis;
    currentMode = (currentMode == TEKS) ? JAM : TEKS;
    Disp.clear();
  }

  switch (currentMode) {
    case TEKS:
      TeksJalan(2, scrollSpeed);
      break;
    case JAM:
      if (currentMillis - lastTimeUpdate >= TIME_UPDATE_INTERVAL) {
        lastTimeUpdate = currentMillis;
        showTime();
      }
      break;
  }
}

//--------------------------
// Fungsi-fungsi tambahan

void readSettingsFromEEPROM() {
  // Baca teks dari EEPROM
  for (int i = 0; i < 50; i++) {
    teks[i] = EEPROM.read(EEPROM_TEXT_START + i);
    if (teks[i] == 0) break;
  }
  if (strlen(teks) == 0) {
    strcpy(teks, "TES TEXT BERJALAN");
  }
  
  // Baca brightness dari EEPROM
  displayBrightness = EEPROM.read(EEPROM_BRIGHTNESS);
  if (displayBrightness < 0 || displayBrightness > 255) {
    displayBrightness = 20; // Nilai default jika tidak valid
  }
  
  // Baca scroll speed dari EEPROM
  scrollSpeed = EEPROM.read(EEPROM_SPEED);
  if (scrollSpeed < 10 || scrollSpeed > 200) {
    scrollSpeed = 50; // Nilai default jika tidak valid
  }
}
void handleNotFound() {
  // Arahkan semua permintaan yang tidak dikenali ke halaman utama
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

void saveSettingsToEEPROM() {
  // Simpan teks
  for (int i = 0; i < strlen(teks); i++) {
    EEPROM.write(EEPROM_TEXT_START + i, teks[i]);
  }
  EEPROM.write(EEPROM_TEXT_START + strlen(teks), 0);
  
  // Simpan brightness
  EEPROM.write(EEPROM_BRIGHTNESS, displayBrightness);
  
  // Simpan scroll speed
  EEPROM.write(EEPROM_SPEED, scrollSpeed);
  
  EEPROM.commit();
}

//--------------------------
// TAMPILKAN SCROLLING TEKS

void TeksJalan(int y, uint8_t kecepatan) {
  static uint32_t pM;
  static uint32_t x;
  int width = Disp.width();
  Disp.setFont(ElektronMart6x12);
  int fullScroll = Disp.textWidth(teks) + width;

  if ((millis() - pM) > kecepatan) {
    pM = millis();
    x = (x < fullScroll) ? x + 1 : 0;
    Disp.clear();
    Disp.drawText(width - x, y, teks);
  }
}

//--------------------------
// TAMPILKAN JAM

void showTime() {
  static bool showColon = true;
  static uint8_t lastSecond = 0;
  
  if (!rtcValid) {
    Disp.clear();
    int textWidth = Disp.textWidth("ERROR");
    int startX = (Disp.width() - textWidth) / 2;
    Disp.drawText(startX, 2, "ERROR");
    return;
  }
  
  DateTime now = rtc.now();
  
  // Update status kedipan setiap detik
  if (now.second() != lastSecond) {
    lastSecond = now.second();
    showColon = !showColon;
  }
  
  // Format jam dan menit
  char timeStr[6];
  snprintf(timeStr, sizeof(timeStr), "%02d %02d", now.hour(), now.minute());
  
  // Hitung lebar total
  int hourWidth = Disp.textWidth(timeStr, 2); // 2 digit pertama (jam)
  int colonWidth = Disp.textWidth(":");
  int minuteWidth = Disp.textWidth(timeStr + 3, 2); // 2 digit terakhir (menit)
  int totalWidth = hourWidth + colonWidth + minuteWidth + 4; // Tambahkan 4 pixel untuk spasi
  
  // Hitung posisi awal untuk senter
  int startX = (Disp.width() - totalWidth) / 2;
  
  Disp.clear();
  
  // Tampilkan jam
  Disp.drawText(startX, 2, timeStr, 2);
  startX += hourWidth + 2; // Tambah spasi 2 pixel setelah jam
  
  // Tampilkan titik dua (dengan spasi sebelum dan sesudah)
  if (showColon) {
    Disp.drawText(startX, 2, ":");
  }
  startX += colonWidth + 2; // Tambah spasi 2 pixel setelah titik dua
  
  // Tampilkan menit
  Disp.drawText(startX, 2, timeStr + 3, 2);
}

//--------------------------
// WEB SERVER HANDLERS

void handleRoot() {
  String currentTime = "ERROR";
  String rtcStatus = "ERROR";
  
  if (rtcValid) {
    DateTime now = rtc.now();
    if (isValidTime(now)) {
      char timeBuffer[6];
      snprintf(timeBuffer, sizeof(timeBuffer), "%02d:%02d", now.hour(), now.minute());
      currentTime = String(timeBuffer);
      rtcStatus = "OK";
    } else {
      rtcStatus = "INVALID TIME";
    }
  } else {
    rtcStatus = "RTC NOT FOUND";
  }
  
  String message = server.arg("message");
  
  String html = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>P10 Display Control Panel</title>
  <style>
    @import url('https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700&display=swap');
    
    :root {
      --primary: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      --primary-solid: #667eea;
      --secondary: #00d4aa;
      --accent: #ff6b6b;
      --background: #0a0e1a;
      --surface: #1a1f36;
      --surface-light: #2a2f46;
      --text-primary: #ffffff;
      --text-secondary: #a0a9c0;
      --text-muted: #6b7280;
      --success: #10b981;
      --warning: #f59e0b;
      --error: #ef4444;
      --border: #374151;
      --shadow: 0 20px 25px -5px rgba(0, 0, 0, 0.5), 0 10px 10px -5px rgba(0, 0, 0, 0.3);
    }
    
    * {
      box-sizing: border-box;
      margin: 0;
      padding: 0;
    }
    
    html {
      scroll-behavior: smooth;
    }
    
    body {
      font-family: 'Inter', -apple-system, BlinkMacSystemFont, sans-serif;
      background: var(--background);
      color: var(--text-primary);
      line-height: 1.6;
      min-height: 100vh;
      overflow-x: hidden;
    }
    
    /* Animated background */
    body::before {
      content: '';
      position: fixed;
      top: 0;
      left: 0;
      width: 100%;
      height: 100%;
      background: 
        radial-gradient(circle at 20% 80%, rgba(120, 119, 198, 0.1) 0%, transparent 50%),
        radial-gradient(circle at 80% 20%, rgba(255, 107, 107, 0.1) 0%, transparent 50%),
        radial-gradient(circle at 40% 40%, rgba(0, 212, 170, 0.1) 0%, transparent 50%);
      z-index: -1;
      animation: float 20s ease-in-out infinite;
    }
    
    @keyframes float {
      0%, 100% { transform: translate(0, 0) rotate(0deg); }
      33% { transform: translate(-20px, -20px) rotate(1deg); }
      66% { transform: translate(20px, -10px) rotate(-1deg); }
    }
    
    .container {
      max-width: 1000px;
      margin: 0 auto;
      padding: 2rem;
      position: relative;
      z-index: 1;
    }
    
    .header {
      text-align: center;
      margin-bottom: 3rem;
      position: relative;
    }
    
    .header h1 {
      font-size: 2.5rem;
      font-weight: 700;
      background: var(--primary);
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
      background-clip: text;
      margin-bottom: 0.5rem;
      position: relative;
    }
    
    .header p {
      color: var(--text-secondary);
      font-size: 1.1rem;
      font-weight: 400;
    }
    
    /* Glass morphism cards */
    .card {
      background: rgba(26, 31, 54, 0.7);
      backdrop-filter: blur(20px);
      border: 1px solid rgba(255, 255, 255, 0.1);
      border-radius: 20px;
      padding: 2rem;
      margin-bottom: 2rem;
      box-shadow: var(--shadow);
      transition: all 0.3s ease;
      position: relative;
      overflow: hidden;
    }
    
    .card::before {
      content: '';
      position: absolute;
      top: 0;
      left: -100%;
      width: 100%;
      height: 100%;
      background: linear-gradient(90deg, transparent, rgba(255,255,255,0.1), transparent);
      transition: left 0.5s;
    }
    
    .card:hover::before {
      left: 100%;
    }
    
    .card:hover {
      transform: translateY(-5px);
      border-color: rgba(102, 126, 234, 0.3);
    }
    
    /* Status indicators */
    .status-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
      gap: 1.5rem;
      margin-bottom: 2rem;
    }
    
    .status-item {
      background: rgba(42, 47, 70, 0.5);
      border-radius: 16px;
      padding: 1.5rem;
      display: flex;
      align-items: center;
      gap: 1rem;
      transition: all 0.3s ease;
      position: relative;
    }
    
    .status-item:hover {
      background: rgba(42, 47, 70, 0.8);
      transform: scale(1.02);
    }
    
    .status-icon {
      width: 48px;
      height: 48px;
      border-radius: 12px;
      display: flex;
      align-items: center;
      justify-content: center;
      font-size: 1.5rem;
      background: var(--primary);
      color: white;
      flex-shrink: 0;
    }
    
    .status-content {
      flex: 1;
    }
    
    .status-label {
      font-size: 0.875rem;
      color: var(--text-muted);
      margin-bottom: 0.25rem;
      text-transform: uppercase;
      letter-spacing: 0.5px;
      font-weight: 500;
    }
    
    .status-value {
      font-size: 1.125rem;
      font-weight: 600;
      color: var(--text-primary);
    }
    
    /* Form styling */
    .form-section {
      margin-bottom: 2rem;
    }
    
    .section-title {
      font-size: 1.25rem;
      font-weight: 600;
      color: var(--text-primary);
      margin-bottom: 1.5rem;
      display: flex;
      align-items: center;
      gap: 0.5rem;
    }
    
    .section-title::before {
      content: '';
      width: 4px;
      height: 24px;
      background: var(--primary);
      border-radius: 2px;
    }
    
    .form-group {
      margin-bottom: 1.5rem;
    }
    
    .form-label {
      display: block;
      font-size: 0.875rem;
      font-weight: 500;
      color: var(--text-secondary);
      margin-bottom: 0.5rem;
      text-transform: uppercase;
      letter-spacing: 0.5px;
    }
    
    .form-input {
      width: 100%;
      padding: 1rem;
      background: rgba(42, 47, 70, 0.5);
      border: 2px solid rgba(255, 255, 255, 0.1);
      border-radius: 12px;
      color: var(--text-primary);
      font-size: 1rem;
      transition: all 0.3s ease;
      font-family: inherit;
    }
    
    .form-input:focus {
      outline: none;
      border-color: var(--primary-solid);
      background: rgba(42, 47, 70, 0.8);
      box-shadow: 0 0 0 4px rgba(102, 126, 234, 0.1);
    }
    
    /* Custom range slider */
    .range-container {
      position: relative;
      margin: 1rem 0;
    }
    
    .range-input {
      width: 100%;
      height: 8px;
      background: rgba(42, 47, 70, 0.8);
      border-radius: 4px;
      outline: none;
      -webkit-appearance: none;
      position: relative;
    }
    
    .range-input::-webkit-slider-thumb {
      -webkit-appearance: none;
      width: 24px;
      height: 24px;
      background: var(--primary);
      border-radius: 50%;
      cursor: pointer;
      box-shadow: 0 4px 12px rgba(102, 126, 234, 0.4);
      transition: all 0.2s ease;
    }
    
    .range-input::-webkit-slider-thumb:hover {
      transform: scale(1.2);
      box-shadow: 0 6px 16px rgba(102, 126, 234, 0.6);
    }
    
    .range-labels {
      display: flex;
      justify-content: space-between;
      margin-top: 0.5rem;
    }
    
    .range-label {
      font-size: 0.75rem;
      color: var(--text-muted);
    }
    
    /* Button styles */
    .btn {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      gap: 0.5rem;
      padding: 1rem 2rem;
      font-size: 0.875rem;
      font-weight: 600;
      text-transform: uppercase;
      letter-spacing: 0.5px;
      border: none;
      border-radius: 12px;
      cursor: pointer;
      transition: all 0.3s ease;
      text-decoration: none;
      position: relative;
      overflow: hidden;
    }
    
    .btn-primary {
      background: var(--primary);
      color: white;
      box-shadow: 0 4px 15px rgba(102, 126, 234, 0.4);
    }
    
    .btn-primary:hover {
      transform: translateY(-2px);
      box-shadow: 0 8px 25px rgba(102, 126, 234, 0.6);
    }
    
    .btn-secondary {
      background: rgba(42, 47, 70, 0.8);
      color: var(--text-primary);
      border: 2px solid rgba(255, 255, 255, 0.1);
    }
    
    .btn-secondary:hover {
      background: rgba(42, 47, 70, 1);
      border-color: rgba(255, 255, 255, 0.2);
      transform: translateY(-2px);
    }
    
    .btn-danger {
      background: linear-gradient(135deg, var(--error) 0%, #dc2626 100%);
      color: white;
      box-shadow: 0 4px 15px rgba(239, 68, 68, 0.4);
    }
    
    .btn-danger:hover {
      transform: translateY(-2px);
      box-shadow: 0 8px 25px rgba(239, 68, 68, 0.6);
    }
    
    .btn-group {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 1rem;
      margin-top: 2rem;
    }
    
    /* Notification styles */
    .notification {
      background: rgba(16, 185, 129, 0.1);
      border: 1px solid rgba(16, 185, 129, 0.3);
      border-radius: 12px;
      padding: 1rem;
      margin-bottom: 2rem;
      display: flex;
      align-items: center;
      gap: 0.75rem;
      animation: slideIn 0.3s ease;
    }
    
    .notification-icon {
      font-size: 1.25rem;
    }
    
    .notification-success {
      background: rgba(16, 185, 129, 0.1);
      border-color: rgba(16, 185, 129, 0.3);
      color: var(--success);
    }
    
    .notification-error {
      background: rgba(239, 68, 68, 0.1);
      border-color: rgba(239, 68, 68, 0.3);
      color: var(--error);
    }
    
    @keyframes slideIn {
      from {
        opacity: 0;
        transform: translateY(-20px);
      }
      to {
        opacity: 1;
        transform: translateY(0);
      }
    }
    
    /* Error section */
    .error-section {
      background: rgba(239, 68, 68, 0.1);
      border: 2px solid rgba(239, 68, 68, 0.3);
      border-radius: 16px;
      padding: 2rem;
    }
    
    .error-title {
      color: var(--error);
      font-size: 1.25rem;
      font-weight: 600;
      margin-bottom: 1rem;
      display: flex;
      align-items: center;
      gap: 0.5rem;
    }
    
    .error-description {
      color: var(--text-secondary);
      margin-bottom: 1.5rem;
      line-height: 1.6;
    }
    
    /* Responsive design */
    @media (max-width: 768px) {
      .container {
        padding: 1rem;
      }
      
      .header h1 {
        font-size: 2rem;
      }
      
      .status-grid {
        grid-template-columns: 1fr;
      }
      
      .btn-group {
        grid-template-columns: 1fr;
      }
      
      .card {
        padding: 1.5rem;
      }
      
      .status-item {
        padding: 1rem;
      }
    }
    
    @media (max-width: 480px) {
      .header h1 {
        font-size: 1.75rem;
      }
      
      .section-title {
        font-size: 1.125rem;
      }
    }
    
    /* Loading animation */
    .loading {
      display: inline-block;
      width: 16px;
      height: 16px;
      border: 2px solid rgba(255,255,255,.3);
      border-radius: 50%;
      border-top-color: var(--primary-solid);
      animation: spin 1s ease-in-out infinite;
    }
    
    @keyframes spin {
      to { transform: rotate(360deg); }
    }
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
      <h1>P10 Display Control</h1>
      <p>Advanced LED Panel Management System</p>
    </div>
    
    )=====";

  // Notification area
  if (message.length() > 0) {
    html += R"=====(
    <div class="notification notification-success">
      <span class="notification-icon">✅</span>
      <span>)=====";
    html += message;
    html += R"=====(</span>
    </div>
    )=====";
  }

  // Status cards
  html += R"=====(
    <div class="status-grid">
      <div class="status-item">
        <div class="status-icon">🕐</div>
        <div class="status-content">
          <div class="status-label">RTC Status</div>
          <div class="status-value">)=====";
  html += rtcStatus;
  html += R"=====(</div>
        </div>
      </div>
      
      <div class="status-item">
        <div class="status-icon">⏰</div>
        <div class="status-content">
          <div class="status-label">Current Time</div>
          <div class="status-value">)=====";
  html += currentTime;
  html += R"=====(</div>
        </div>
      </div>
      
      <div class="status-item">
        <div class="status-icon">📺</div>
        <div class="status-content">
          <div class="status-label">Display Mode</div>
          <div class="status-value">)=====";
  html += (currentMode == TEKS ? "Scrolling Text" : "Clock");
  html += R"=====(</div>
        </div>
      </div>
      
      <div class="status-item">
        <div class="status-icon">💡</div>
        <div class="status-content">
          <div class="status-label">Brightness</div>
          <div class="status-value">)=====";
  html += String(displayBrightness);
  html += R"=====(/255</div>
        </div>
      </div>
    </div>
    
    <form action="/set" method="GET">
      <div class="card">
        <div class="form-section">
          <div class="section-title">✏️ Text Configuration</div>
          
          <div class="form-group">
            <label class="form-label">Display Text</label>
            <input type="text" class="form-input" name="text" value=")=====";
  html += String(teks);
  html += R"=====(" maxlength="49" placeholder="Enter your scrolling text here...">
          </div>
          
          <div class="form-group">
            <label class="form-label">Current Text: ")=====";
  html += String(teks);
  html += R"=====(="</label>
          </div>
          
          <div class="form-group">
            <label class="form-label">Scroll Speed: )=====";
  html += String(scrollSpeed);
  html += R"=====(ms</label>
            <div class="range-container">
              <input type="range" class="range-input" name="speed" min="10" max="200" value=")=====";
  html += String(scrollSpeed);
  html += R"=====(">
              <div class="range-labels">
                <span class="range-label">⚡ Fast (10ms)</span>
                <span class="range-label">🐌 Slow (200ms)</span>
              </div>
            </div>
          </div>
        </div>
      </div>
      
      <div class="card">
        <div class="form-section">
          <div class="section-title">🎨 Display Settings</div>
          
          <div class="form-group">
            <label class="form-label">Brightness: )=====";
  html += String(displayBrightness);
  html += R"=====(/255</label>
            <div class="range-container">
              <input type="range" class="range-input" name="brightness" min="0" max="255" value=")=====";
  html += String(displayBrightness);
  html += R"=====(">
              <div class="range-labels">
                <span class="range-label">🌑 Dim (0)</span>
                <span class="range-label">☀️ Bright (255)</span>
              </div>
            </div>
          </div>
        </div>
      </div>
      
      <div class="card">
        <div class="form-section">
          <div class="section-title">⏰ Time Management</div>
          
          <div class="form-group">
            <label class="form-label">Set Current Time</label>
            <input type="time" class="form-input" name="time" step="60">
            <small style="color: var(--text-muted); margin-top: 0.5rem; display: block;">Leave empty to keep current time</small>
          </div>
        </div>
      </div>
      
      <div class="btn-group">
        <button type="submit" class="btn btn-primary">
          <span>💾</span>
          Save All Settings
        </button>
        <button type="button" onclick="location.reload()" class="btn btn-secondary">
          <span>🔄</span>
          Refresh Status
        </button>
      </div>
  )=====";

  // Add RTC error section if needed
  if (!rtcValid) {
    html += R"=====(
      <div class="card error-section">
        <div class="error-title">
          <span>⚠️</span>
          RTC Error Detected
        </div>
        <p class="error-description">
          The Real-Time Clock module is not functioning properly. This may affect time display accuracy. 
          You can attempt to reset the RTC to default time (12:00) to resolve this issue.
        </p>
        <input type="hidden" name="reset_rtc" value="1">
        <button type="submit" class="btn btn-danger">
          <span>🔧</span>
          Reset RTC to 12:00
        </button>
      </div>
    )=====";
  }

  html += R"=====(
    </form>
  </div>

  <script>
    // Real-time range value updates
    document.querySelectorAll('input[type="range"]').forEach(slider => {
      slider.addEventListener('input', function() {
        const label = document.querySelector(`label[for="${this.name}"]`) || 
                     this.closest('.form-group').querySelector('.form-label');
        if (label && this.name === 'speed') {
          label.textContent = `Scroll Speed: ${this.value}ms`;
        } else if (label && this.name === 'brightness') {
          label.textContent = `Brightness: ${this.value}/255`;
        }
      });
    });

    // Form submission loading state
    document.querySelector('form').addEventListener('submit', function() {
      const submitBtn = document.querySelector('.btn-primary');
      const originalText = submitBtn.innerHTML;
      submitBtn.innerHTML = '<span class="loading"></span> Saving...';
      submitBtn.disabled = true;
    });

    // Auto-refresh status every 30 seconds
    setTimeout(() => {
      location.reload();
    }, 30000);
  </script>
</body>
</html>
)=====";

  server.send(200, "text/html", html);
}

void handleSet() {
  String message = "";
  
  // Reset RTC jika diminta
  if (server.arg("reset_rtc") == "1") {
    Serial.println("Reset RTC diminta...");
    rtc.adjust(DateTime(2024, 1, 1, 12, 0, 0));
    delay(100);
    DateTime testTime = rtc.now();
    if (isValidTime(testTime)) {
      rtcValid = true;
      message += "RTC berhasil di-reset ke 12:00. ";
      Serial.println("RTC reset berhasil!");
    } else {
      message += "RTC reset gagal! ";
      Serial.println("RTC reset gagal!");
    }
  }
  
  // Update teks
  if (server.arg("text") != "") {
    server.arg("text").toCharArray(teks, 50);
    message += "Teks diupdate. ";
  }

  // Update kecepatan scroll
  if (server.arg("speed") != "") {
    scrollSpeed = server.arg("speed").toInt();
    if (scrollSpeed < 10) scrollSpeed = 10;
    if (scrollSpeed > 200) scrollSpeed = 200;
    message += "Kecepatan diupdate ke " + String(scrollSpeed) + "ms. ";
  }

  // Update brightness
  if (server.arg("brightness") != "") {
    displayBrightness = server.arg("brightness").toInt();
    if (displayBrightness < 0) displayBrightness = 0;
    if (displayBrightness > 255) displayBrightness = 255;
    Disp.setBrightness(displayBrightness);
    message += "Kecerahan diupdate ke " + String(displayBrightness) + ". ";
  }

  // Update waktu
  if (server.hasArg("time") && server.arg("time") != "") {
    String timeStr = server.arg("time");
    
    if (isValidTimeFormat(timeStr)) {
      int hour = timeStr.substring(0, 2).toInt();
      int minute = timeStr.substring(3, 5).toInt();
      
      if (rtcValid) {
        DateTime now = rtc.now();
        rtc.adjust(DateTime(now.year(), now.month(), now.day(), hour, minute, 0));
        message += "Jam diatur ke " + timeStr + ". ";
        Serial.printf("Jam diatur: %02d:%02d\n", hour, minute);
      } else {
        message += "Tidak dapat mengatur jam - RTC error! ";
      }
    } else {
      message += "Format waktu tidak valid! ";
    }
  }

  // Simpan semua pengaturan ke EEPROM
  if (server.arg("text") != "" || server.arg("speed") != "" || server.arg("brightness") != "") {
    saveSettingsToEEPROM();
    message += "Pengaturan disimpan. ";
  }

  server.sendHeader("Location", "/?message=" + message);
  server.send(303);
}

//--------------------------
// VALIDASI FORMAT WAKTU

bool isValidTime(DateTime time) {
  int h = time.hour();
  int m = time.minute();
  int s = time.second();
  return (h >= 0 && h <= 23 && m >= 0 && m <= 59 && s >= 0 && s <= 59);
}

bool isValidTimeFormat(String timeStr) {
  if (timeStr.length() != 5) return false;
  if (timeStr.charAt(2) != ':') return false;
  
  String hourStr = timeStr.substring(0, 2);
  String minuteStr = timeStr.substring(3, 5);
  
  if (!isNumeric(hourStr) || !isNumeric(minuteStr)) return false;
  
  int hour = hourStr.toInt();
  int minute = minuteStr.toInt();
  
  return (hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59);
}

bool isNumeric(String str) {
  for (int i = 0; i < str.length(); i++) {
    if (!isDigit(str.charAt(i))) return false;
  }
  return true;
}
