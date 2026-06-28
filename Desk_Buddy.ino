#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>     
#include <DNSServer.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h> 
#include <SPI.h>
#include <driver/gpio.h> 
#include <time.h>
#include <ArduinoOTA.h> 
#include <DHT.h> 

// ==========================================
// 1. HARDWARE PIN DEFINITIONS (ESP32-C3-Zero)
// ==========================================
#define TOUCH_1       3  // Capacitive Touch Pad (Wakeup Source)
#define DHT_PWR_PIN   4  // DHT11 VCC (Dynamic Power)
#define DHT_PIN       5  // DHT11 Data Pin
#define SLEEP_LED_PIN 8  // TFT Backlight (BC558 PNP: LOW = ON, HIGH = OFF)

#define DHTTYPE DHT11
DHT dht(DHT_PIN, DHTTYPE);

// ==========================================
// 2. GLOBAL VARIABLES & OBJECTS
// ==========================================
TFT_eSPI tft = TFT_eSPI(); 
TFT_eSprite spr = TFT_eSprite(&tft); 

Preferences preferences;
WebServer server(80); 
DNSServer dnsServer;

// Color Palette
uint16_t BRAND_BLUE   = tft.color565(4, 132, 248); 
uint16_t COLOR_SUN    = tft.color565(255, 215, 0);   
uint16_t COLOR_CLOUD  = tft.color565(180, 180, 185); 
uint16_t COLOR_RAIN   = tft.color565(100, 180, 255); 

bool isConfigMode = false;
bool isThrottled = false;
float currentChipTemp = 0.0;
int currentCpuFreq = 160; 

// Sensor Data
volatile float inTemp = 0.0;
volatile float inHum = 0.0;

// Weather Data
String owmApiKey = "97c7487fa37e19c27db5c6186ff96635";
float outTemp = 0.0;
int outHumidity = 0;
String weatherCondition = "Loading...";

int forecastTemps[3] = {0, 0, 0};
String forecastConds[3] = {"Clear", "Clear", "Clear"};
String forecastDays[3] = {"---", "---", "---"};

const char* ntpServer = "time.google.com";
const long gmtOffset_sec = 19800; 
const int daylightOffset_sec = 0;
bool use24HourFormat = false;

// UI State Management
int currentUI = 2; // Boot directly to Robot Eyes
unsigned long uiTimer = 0; 
unsigned long lastSlowTick = 0;
int weatherCounter = 0;

// Touch State
bool ttp1_wasPressed = false;
unsigned long ttp1_pressTime = 0;

// Robot Eyes Config
uint16_t currentEyeColor = TFT_WHITE;
unsigned long lastColorChange = 0;

// ==========================================
// 3. CAPTIVE PORTAL HTML
// ==========================================
const char dashboard_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Desk_Buddy</title>
    <style>
        :root { --bg: #0484f8; --glass: rgba(255, 255, 255, 0.15); --border: rgba(255, 255, 255, 0.3); }
        body { font-family: -apple-system, system-ui, sans-serif; background-color: var(--bg); color: white; margin: 0; padding: 20px; line-height: 1.6; }
        h1 { text-align: center; font-size: 2.2em; margin-bottom: 30px; font-weight: 800; letter-spacing: 2px; }
        h2 { font-size: 1.1em; border-bottom: 1px solid var(--border); padding-bottom: 10px; margin-bottom: 20px; display: flex; align-items: center; gap: 10px; text-transform: uppercase; color: white;}
        .container { max-width: 1200px; margin: 0 auto; display: grid; gap: 20px; grid-template-columns: 1fr; }
        @media (min-width: 800px) { .container { grid-template-columns: 1fr 1fr; } .full-width { grid-column: 1 / -1; } }
        .panel { background: var(--glass); backdrop-filter: blur(12px); -webkit-backdrop-filter: blur(12px); border: 1px solid var(--border); border-radius: 16px; padding: 25px; }
        .telemetry-box { background: rgba(0,0,0,0.2); border-radius: 8px; padding: 15px; text-align: center; font-family: monospace; font-size: 14px; margin-bottom: 20px; border: 1px dashed var(--border); }
        svg { width: 22px; height: 22px; fill: currentColor; flex-shrink: 0; }
        label { display: block; font-size: 0.85em; font-weight: 600; text-transform: uppercase; margin-bottom: 8px; }
        ::placeholder { color: white !important; opacity: 1 !important; font-weight: 500; }
        input, select { width: 100%; padding: 14px; box-sizing: border-box; background: rgba(0, 0, 0, 0.2); border: 1px solid var(--border); border-radius: 8px; color: white; margin-bottom: 15px; outline: none; }
        input:focus, select:focus { background: rgba(0, 0, 0, 0.3); border-color: white; }
        button { width: 100%; padding: 14px; background: white; color: var(--bg); border: none; border-radius: 8px; font-size: 1.1em; font-weight: 700; cursor: pointer; display: flex; justify-content: center; align-items: center; gap: 8px; margin-bottom: 10px; text-transform: uppercase;}
        button.secondary { background: rgba(0,0,0,0.3); color: white; }
        button.danger { background: #ef4444; color: white; margin-top: 10px;}
        .input-group { position: relative; }
        .input-group svg { position: absolute; right: 14px; top: 14px; cursor: pointer; }
        #networkList { display: none; background: rgba(0,0,0,0.4); border-radius: 8px; border: 1px solid var(--border); margin-bottom: 15px; max-height: 220px; overflow-y: auto;}
        .net-item { padding: 14px 15px; cursor: pointer; border-bottom: 1px solid rgba(255,255,255,0.1); display: flex; align-items: center; gap: 12px; }
        .radio-group { display: flex; gap: 10px; margin-bottom: 20px; }
        .radio-group label { flex: 1; cursor: pointer; position: relative; }
        .radio-group input { position: absolute; opacity: 0; }
        .radio-group span { display: block; text-align: center; padding: 14px; background: rgba(0,0,0,0.2); border: 1px solid var(--border); border-radius: 8px; color: white;}
        .radio-group input:checked + span { background: white; color: var(--bg); font-weight: bold;}
        .flex-row { display: flex; gap: 15px; } .flex-row > * { flex: 1; margin-bottom: 0; }
    </style>
</head>
<body>
<h1>DESK_BUDDY</h1>
<form id="dashboardForm" action="/save-settings" method="POST">
<div class="container">
    <div class="panel">
        <h2>Network Setup</h2>
        <div class="telemetry-box" id="telemetry">Fetching Data...</div>
        <div style="position: relative;">
            <input type="text" name="ssid" id="ssid" placeholder="Wi-Fi Network" value="__SAVED_SSID__" __SSID_READONLY__ required>
            <button type="button" id="unlockBtn" onclick="unlockWifi()" style="display: __UNLOCK_BTN_DISPLAY__; position: absolute; right: 6px; top: 6px; width: auto; padding: 8px 14px; font-size: 0.85em; background: rgba(0,0,0,0.5);">EDIT</button>
        </div>
        <div id="networkList"></div>
        <div class="input-group">
            <input type="password" name="password" id="password" placeholder="Wi-Fi Password" >
            <svg onclick="togglePassword()" id="eyeIcon" viewBox="0 0 24 24"><path d="M12 4.5C7 4.5 2.73 7.61 1 12c1.73 4.39 6 7.5 11 7.5s9.27-3.11 11-7.5c-1.73-4.39-6-7.5-11-7.5zM12 17c-2.76 0-5-2.24-5-5s2.24-5 5-5 5 2.24 5 5-2.24 5-5 5zm0-8c-1.66 0-3 1.34-3 3s1.34 3 3 3 3-1.34 3-3-1.34-3-3-3z"/></svg>
        </div>
        <button type="button" class="secondary" id="scanBtn" onclick="scanNetworks()">Scan Wi-Fi</button>

        <label style="margin-top: 25px;">Location Data</label>
        <button type="button" class="secondary" onclick="getLocation()">Auto-Fill GPS</button>
        <input type="text" name="city" placeholder="City Name">
        <div class="flex-row">
            <input type="text" name="lat" id="latInput" placeholder="Latitude">
            <input type="text" name="lon" id="lonInput" placeholder="Longitude">
        </div>
    </div>

    <div class="panel">
        <h2>System Setup</h2>
        <label>Time Format</label>
        <div class="radio-group">
            <label><input type="radio" name="time_format" value="12" checked><span>12 HR</span></label>
            <label><input type="radio" name="time_format" value="24"><span>24 HR</span></label>
        </div>
    </div>

    <div class="full-width">
        <button type="submit">SAVE SETTINGS</button>
        <button type="button" class="danger" onclick="rebootClock()">REBOOT CLOCK</button>
    </div>
</div>
</form>

<script>
    function unlockWifi() {
        const ssidInput = document.getElementById('ssid');
        ssidInput.readOnly = false;
        ssidInput.style.opacity = '1';
        ssidInput.style.pointerEvents = 'auto';
        ssidInput.value = '';
        document.getElementById('unlockBtn').style.display = 'none';
    }

    function togglePassword() {
        const p = document.getElementById("password"); 
        const e = document.getElementById("eyeIcon");
        if (p.type === "password") { 
            p.type = "text"; 
            e.innerHTML = `<path d="M12 7c2.76 0 5 2.24 5 5 0 .65-.13 1.26-.36 1.83l2.92 2.92c1.51-1.26 2.7-2.89 3.43-4.75-1.73-4.39-6-7.5-11-7.5-1.4 0-2.74.25-3.98.7l2.16 2.16C10.74 7.13 11.35 7 12 7zM2 4.27l2.28 2.28.46.46C3.08 8.3 1.78 10.02 1 12c1.73 4.39 6 7.5 11 7.5 1.55 0 3.03-.3 4.38-.84l.42.42L19.73 22 21 20.73 3.27 3 2 4.27zM7.53 9.8l1.55 1.55c-.05.21-.08.43-.08.65 0 1.66 1.34 3 3 3 .22 0 .44-.03.65-.08l1.55 1.55c-.67.33-1.41.53-2.2.53-2.76 0-5-2.24-5-5 0-.79.2-1.53.53-2.2zm4.31-.78l3.15 3.15.02-.16c0-1.66-1.34-3-3-3l-.17.01z"/>`;
        } else { 
            p.type = "password"; 
            e.innerHTML = `<path d="M12 4.5C7 4.5 2.73 7.61 1 12c1.73 4.39 6 7.5 11 7.5s9.27-3.11 11-7.5c-1.73-4.39-6-7.5-11-7.5zM12 17c-2.76 0-5-2.24-5-5s2.24-5 5-5 5 2.24 5 5-2.24 5-5 5zm0-8c-1.66 0-3 1.34-3 3s1.34 3 3 3 3-1.34 3-3-1.34-3-3-3z"/>`;
        }
    }

    function scanNetworks() {
        unlockWifi(); 
        const btn = document.getElementById('scanBtn');
        const list = document.getElementById('networkList');
        btn.innerHTML = `Scanning...`;
        
        const svgWifi = `<svg viewBox="0 0 24 24" style="width:20px;height:20px;fill:currentColor;"><path d="M12 3C7.8 3 3.7 4.4.4 6.9c-.5.4-.6 1.1-.2 1.6l1.5 1.7c.4.4 1 .5 1.4.1C5.7 8.3 8.8 7.3 12 7.3c3.2 0 6.3 1 8.9 2.9.4.3 1 .2 1.4-.1l1.5-1.7c.4-.5.3-1.2-.2-1.6C20.3 4.4 16.2 3 12 3zm0 5.4c-2.4 0-4.7.7-6.5 2-.4.3-.5 1-.1 1.4l1.5 1.7c.3.4.9.4 1.3.1C9.4 12.6 10.7 12 12 12s2.6.6 3.8 1.5c.4.3 1 .3 1.3-.1l1.5-1.7c.3-.4.3-1.1-.1-1.4-1.8-1.3-4.1-2-6.5-2zm0 5.3c-1 0-1.9.3-2.6.8-.4.3-.4 1-.1 1.3l1.5 1.7c.3.4.9.4 1.3.1.5-.4 1.2-.4 1.7 0 .4.3 1 .2 1.3-.1l1.5-1.7c.4-.4.3-1-.1-1.3-.7-.5-1.6-.8-2.5-.8zm0 5.3c-.8 0-1.5.7-1.5 1.5s.7 1.5 1.5 1.5 1.5-.7 1.5-1.5-.7-1.5-1.5-1.5z"/></svg>`;

        fetch('/scan').then(r => r.json()).then(networks => {
            list.innerHTML = ""; list.style.display = "block";
            if(networks.length === 0) {
                list.innerHTML = "<div class='net-item'>No networks found</div>";
            } else {
                let seen = new Set();
                networks.forEach(net => {
                    if(!seen.has(net.ssid)) {
                        seen.add(net.ssid);
                        let div = document.createElement('div'); div.className = 'net-item'; 
                        div.innerHTML = svgWifi + " <span style='flex-grow:1; margin-left:10px;'>" + net.ssid + "</span> <span style='color:#9ca3af; font-size:0.85em;'>" + net.rssi + " dBm</span>";
                        div.onclick = () => { document.getElementById('ssid').value = net.ssid; list.style.display = "none"; };
                        list.appendChild(div);
                    }
                });
            }
            btn.innerHTML = `Scan Wi-Fi`;
        });
    }

    function getLocation() {
        if (navigator.geolocation) {
            navigator.geolocation.getCurrentPosition(
                pos => { document.getElementById("latInput").value = pos.coords.latitude.toFixed(4); document.getElementById("lonInput").value = pos.coords.longitude.toFixed(4); }, 
                err => alert("Enable Location Services!")
            );
        }
    }

    document.getElementById('dashboardForm').addEventListener('submit', function(e) {
        e.preventDefault();
        fetch('/save-settings', { method: 'POST', body: new URLSearchParams(new FormData(this)) })
        .then(r => { if(r.ok) { alert("Settings Saved! Clock Rebooting..."); location.reload(); }});
    });

    function rebootClock() {
        if (confirm("Restart Desk_Buddy?")) {
            fetch('/reboot', { method: 'POST' }).then(() => {
                setTimeout(() => { location.reload(); }, 3000);
            });
        }
    }

    setInterval(() => {
        fetch('/api/data').then(r => r.json()).then(data => {
            document.getElementById('telemetry').innerHTML = `CPU: ${data.cpu}MHz | CORE: ${data.temp}&deg;C | SIGNAL: ${data.rssi} dBm`;
        });
    }, 5000);
</script>
</body>
</html>
)rawliteral";

// ==========================================
// 4. API & SLEEP FUNCTIONS
// ==========================================
void fetchWeather() {
  if (WiFi.status() == WL_CONNECTED) {
    preferences.begin("clock_prefs", true);
    String city = preferences.getString("city", "Thiruvananthapuram,IN");
    String lat = preferences.getString("lat", "");
    String lon = preferences.getString("lon", "");
    preferences.end();
    
    String baseParams = "";
    if (lat != "" && lon != "") baseParams = "lat=" + lat + "&lon=" + lon;
    else baseParams = "q=" + city;
    baseParams += "&units=metric&appid=" + owmApiKey;

    HTTPClient http;
    
    // 1. Fetch Current Weather
    http.begin("http://api.openweathermap.org/data/2.5/weather?" + baseParams);
    if (http.GET() == HTTP_CODE_OK) {
      JsonDocument doc;
      if (!deserializeJson(doc, http.getString())) {
        outTemp = doc["main"]["temp"];
        outHumidity = doc["main"]["humidity"];
        weatherCondition = doc["weather"][0]["main"].as<String>();
      }
    }
    http.end();
    yield(); // Prevent Watchdog Timer reset

    // 2. Fetch 3-Day Forecast
    http.begin("http://api.openweathermap.org/data/2.5/forecast?" + baseParams + "&cnt=26");
    if (http.GET() == HTTP_CODE_OK) {
      JsonDocument filter;
      filter["list"][0]["main"]["temp"] = true;
      filter["list"][0]["weather"][0]["main"] = true;
      filter["list"][0]["dt"] = true;

      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
      
      if (!error) {
        JsonArray list = doc["list"];
        int indices[3] = {8, 16, 24}; 
        
        for(int i = 0; i < 3; i++) {
          if(list.size() > indices[i]) {
            forecastTemps[i] = list[indices[i]]["main"]["temp"];
            forecastConds[i] = list[indices[i]]["weather"][0]["main"].as<String>();
            long dt = list[indices[i]]["dt"];
            
            time_t t = dt + gmtOffset_sec;
            struct tm *ptm = gmtime(&t);
            char dayStr[4];
            strftime(dayStr, 4, "%a", ptm);
            String d = String(dayStr);
            d.toUpperCase();
            forecastDays[i] = d;
          }
        }
      }
    }
    http.end();
  }
}

void goToSleep() {
  // 1. Draw Sleep Message
  spr.fillSprite(TFT_BLACK);
  spr.setTextColor(BRAND_BLUE, TFT_BLACK);
  spr.setTextFont(4);
  spr.setTextDatum(MC_DATUM);
  spr.drawString("GOING TO", 64, 50);
  spr.drawString("SLEEP", 64, 80);
  spr.pushSprite(0, 0); 
  
  // 2. Hold message on screen briefly
  delay(1500);

  // 3. Hardware Sleep (TFT Internal Driver)
  tft.writecommand(0x10); // Sleep In
  delay(10);
  tft.writecommand(0x28); // Display Off

  // 4. Cut Backlight Power (BC558 PNP Logic: HIGH = OFF)
  digitalWrite(SLEEP_LED_PIN, HIGH); 
  gpio_hold_en((gpio_num_t)SLEEP_LED_PIN);

  // 5. Cut DHT11 Power
  digitalWrite(DHT_PWR_PIN, LOW);
  gpio_hold_en((gpio_num_t)DHT_PWR_PIN);

  // 6. Enter Deep Sleep on ESP32-C3
  esp_deep_sleep_enable_gpio_wakeup(1ULL << TOUCH_1, ESP_GPIO_WAKEUP_GPIO_HIGH);
  esp_deep_sleep_start();
}

// ==========================================
// 5. UI DRAWING
// ==========================================
void drawWeatherIcon(int cx, int cy, String condition) {
  condition.toLowerCase(); 
  if (condition == "clear") spr.fillCircle(cx, cy, 10, COLOR_SUN);
  else if (condition == "clouds") {
    spr.fillCircle(cx - 6, cy + 2, 6, COLOR_CLOUD); spr.fillCircle(cx + 2, cy - 4, 8, COLOR_CLOUD);
    spr.fillCircle(cx + 9, cy + 2, 6, COLOR_CLOUD); spr.fillRect(cx - 6, cy - 1, 15, 9, COLOR_CLOUD); 
  }
  else if (condition == "rain" || condition == "drizzle") {
    spr.fillCircle(cx - 6, cy + 2, 6, COLOR_CLOUD); spr.fillCircle(cx + 2, cy - 4, 8, COLOR_CLOUD);
    spr.fillCircle(cx + 9, cy + 2, 6, COLOR_CLOUD); spr.fillRect(cx - 6, cy - 1, 15, 9, COLOR_CLOUD);
    spr.drawLine(cx - 4, cy + 8, cx - 7, cy + 14, COLOR_RAIN); spr.drawLine(cx + 2, cy + 8, cx - 1,  cy + 14, COLOR_RAIN);
    spr.drawLine(cx + 8, cy + 8, cx + 5, cy + 14, COLOR_RAIN);
  }
  else if (condition == "thunderstorm") {
    spr.fillCircle(cx - 6, cy + 2, 6, COLOR_CLOUD); spr.fillCircle(cx + 2, cy - 4, 8, COLOR_CLOUD);
    spr.fillCircle(cx + 9, cy + 2, 6, COLOR_CLOUD); spr.fillRect(cx - 6, cy - 1, 15, 9, COLOR_CLOUD);
    spr.fillTriangle(cx + 1, cy + 4, cx + 5, cy + 4, cx - 1, cy + 10, COLOR_SUN);
    spr.fillTriangle(cx + 0, cy + 9, cx + 4, cy + 9, cx - 2, cy + 15, COLOR_SUN);
  }
  else {
    spr.fillCircle(cx - 4, cy - 4, 8, COLOR_SUN); spr.fillCircle(cx + 3, cy + 3, 7, TFT_WHITE);
    spr.fillCircle(cx + 10, cy + 3, 5, TFT_WHITE); spr.fillRect(cx + 3, cy, 7, 10, TFT_WHITE);
  }
}

void drawSmallWeatherIcon(int cx, int cy, String condition) {
  condition.toLowerCase();
  if (condition == "clear") spr.fillCircle(cx, cy, 6, COLOR_SUN);
  else if (condition == "clouds") {
      spr.fillCircle(cx - 4, cy + 1, 4, COLOR_CLOUD); spr.fillCircle(cx + 1, cy - 3, 5, COLOR_CLOUD);
      spr.fillCircle(cx + 6, cy + 1, 4, COLOR_CLOUD); spr.fillRect(cx - 4, cy - 1, 10, 6, COLOR_CLOUD);
  }
  else if (condition == "rain" || condition == "drizzle") {
      spr.fillCircle(cx - 4, cy + 1, 4, COLOR_CLOUD); spr.fillCircle(cx + 1, cy - 3, 5, COLOR_CLOUD);
      spr.fillCircle(cx + 6, cy + 1, 4, COLOR_CLOUD); spr.fillRect(cx - 4, cy - 1, 10, 6, COLOR_CLOUD);
      spr.drawLine(cx - 3, cy + 5, cx - 5, cy + 9, COLOR_RAIN); spr.drawLine(cx + 1, cy + 5, cx - 1, cy + 9, COLOR_RAIN);
      spr.drawLine(cx + 5, cy + 5, cx + 3, cy + 9, COLOR_RAIN);
  }
  else if (condition == "thunderstorm") {
      spr.fillCircle(cx - 4, cy + 1, 4, COLOR_CLOUD); spr.fillCircle(cx + 1, cy - 3, 5, COLOR_CLOUD);
      spr.fillCircle(cx + 6, cy + 1, 4, COLOR_CLOUD); spr.fillRect(cx - 4, cy - 1, 10, 6, COLOR_CLOUD);
      spr.fillTriangle(cx + 1, cy + 3, cx + 4, cy + 3, cx - 1, cy + 8, COLOR_SUN);
      spr.fillTriangle(cx + 0, cy + 7, cx + 3, cy + 7, cx - 2, cy + 12, COLOR_SUN);
  }
  else {
      spr.fillCircle(cx - 3, cy - 3, 5, COLOR_SUN); spr.fillCircle(cx + 2, cy + 2, 4, TFT_WHITE);
      spr.fillCircle(cx + 7, cy + 2, 3, TFT_WHITE); spr.fillRect(cx + 2, cy, 5, 6, TFT_WHITE);
  }
}

void drawMainUI() {
  spr.fillSprite(TFT_BLACK); 
  
  spr.drawLine(0, 36, 128, 36, tft.color565(50, 50, 50)); 
  spr.drawLine(74, 36, 74, 160, tft.color565(50, 50, 50));
  
  int wifiX = 12, wifiY = 17;
  spr.fillCircle(wifiX, wifiY, 1, BRAND_BLUE); spr.drawCircle(wifiX, wifiY, 4, BRAND_BLUE);
  spr.drawCircle(wifiX, wifiY, 8, BRAND_BLUE); spr.fillRect(wifiX - 9, wifiY + 1, 19, 9, TFT_BLACK);

  spr.setTextFont(2);
  spr.setTextColor(BRAND_BLUE, TFT_BLACK);
  spr.setTextDatum(TL_DATUM); 

  spr.drawString(String(WiFi.RSSI()) + "dB", 24, 4);
  
  if (isnan(inTemp)) spr.drawString("ERR", 75, 4);
  else spr.drawString("IN:" + String((int)inTemp) + "C", 75, 4);

  if (isnan(inHum)) spr.drawString(" ", 24, 20);
  else spr.drawString("H:" + String((int)inHum) + "%", 24, 20);

  spr.drawString("CPU:" + String(currentCpuFreq), 75, 20);

  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    char hourStr[3]; char minStr[3];
    if (use24HourFormat) strftime(hourStr, sizeof(hourStr), "%H", &timeinfo);
    else strftime(hourStr, sizeof(hourStr), "%I", &timeinfo);
    strftime(minStr, sizeof(minStr), "%M", &timeinfo);

    spr.setTextFont(4); 
    spr.setTextDatum(MC_DATUM); 
    
    spr.drawString(hourStr, 37, 75); 
    spr.drawString(minStr, 37, 105);
  }

  spr.setTextDatum(MC_DATUM); 
  drawSmallWeatherIcon(101, 54, weatherCondition); 

  spr.setTextFont(2); 
  spr.drawString(String((int)outTemp) + "C", 101, 78);
  
  String cond = weatherCondition;
  if (cond.length() > 7) {
    spr.drawString(cond.substring(0, 6), 101, 105); 
    spr.drawString(cond.substring(6), 101, 120); 
  } else {
    spr.drawString(cond, 101, 110); 
  }

  spr.pushSprite(0, 0); 
}

void drawNestWeatherUI() {
  spr.fillSprite(tft.color565(20, 20, 25)); 
  
  spr.setTextColor(BRAND_BLUE); 
  spr.setTextFont(2);
  spr.setTextDatum(TC_DATUM);
  spr.drawString("TODAY", 64, 15); 
  
  drawWeatherIcon(64, 52, weatherCondition); 
  
  spr.setTextColor(TFT_WHITE, tft.color565(20, 20, 25)); 
  spr.setTextFont(4);
  spr.setTextDatum(MC_DATUM); 
  spr.drawString(String((int)outTemp) + "C", 64, 90); 
  
  String displayCond = weatherCondition; 
  displayCond[0] = toupper(displayCond[0]);
  
  spr.setTextColor(BRAND_BLUE, tft.color565(20, 20, 25)); 
  spr.setTextFont(2);
  spr.drawString(displayCond, 64, 115); 

  int dropX = 35;
  int dropY = 140;
  int r = 4;
  spr.fillCircle(dropX, dropY, r, COLOR_RAIN);
  spr.fillTriangle(dropX - r, dropY, dropX + r, dropY, dropX, dropY - r - 5, COLOR_RAIN);
  
  spr.setTextColor(TFT_WHITE, tft.color565(20, 20, 25)); 
  spr.setTextFont(2);
  spr.setTextDatum(ML_DATUM);
  spr.drawString("HUM " + String(outHumidity) + "%", dropX + 12, dropY - 2);

  spr.pushSprite(0, 0); 
}

void drawForecastUI() {
  spr.fillSprite(tft.color565(20, 20, 25)); 
  
  spr.setTextColor(BRAND_BLUE); 
  spr.setTextFont(2);
  spr.setTextDatum(TC_DATUM);
  spr.drawString("FORECAST", 64, 15); 

  for(int i = 0; i < 3; i++) {
    int yBase = 50 + (i * 35); 
    
    spr.setTextColor(TFT_WHITE, tft.color565(20, 20, 25));
    spr.setTextDatum(ML_DATUM);
    spr.drawString(forecastDays[i], 15, yBase);

    drawSmallWeatherIcon(64, yBase, forecastConds[i]);

    spr.setTextDatum(MR_DATUM);
    spr.drawString(String(forecastTemps[i]) + "C", 113, yBase);
  }

  spr.pushSprite(0, 0); 
}

void drawRobotEyes() {
  spr.fillSprite(TFT_BLACK); 
  
  if (millis() - lastColorChange > 1000) {
      lastColorChange = millis();
      uint16_t allowedColors[] = { TFT_BLUE, TFT_RED, TFT_GREEN, TFT_PURPLE, TFT_VIOLET };
      int randomIndex = random(0, 5); 
      currentEyeColor = allowedColors[randomIndex];
  }
  
  spr.fillCircle(34, 80, 18, currentEyeColor); 
  spr.fillCircle(94, 80, 18, currentEyeColor);

  int moveX = sin(millis() / 400.0) * 8; 
  int moveY = cos(millis() / 300.0) * 6;  
  
  spr.fillCircle(34 + moveX, 80 + moveY, 8, TFT_BLACK); 
  spr.fillCircle(94 + moveX, 80 + moveY, 8, TFT_BLACK);
  
  spr.pushSprite(0, 0); 
}

// ==========================================
// 6. SETUP & MAIN LOOP
// ==========================================
void setup() {
  Serial.begin(115200);
  setCpuFrequencyMhz(160); // Set C3 to max performance

  // Wakeup pin config
  pinMode(TOUCH_1, INPUT_PULLDOWN); 
  
  // Power up TFT Backlight via BC558 (LOW = ON)
  pinMode(SLEEP_LED_PIN, OUTPUT);
  gpio_hold_dis((gpio_num_t)SLEEP_LED_PIN);
  digitalWrite(SLEEP_LED_PIN, LOW); 

  // Power up DHT11 dynamically
  pinMode(DHT_PWR_PIN, OUTPUT);
  gpio_hold_dis((gpio_num_t)DHT_PWR_PIN);
  digitalWrite(DHT_PWR_PIN, HIGH);
  delay(100); // Sensor boot time
  dht.begin(); 

  // Init TFT Screen
  tft.init(); 
  tft.setRotation(0); 
  spr.createSprite(128, 160); 

  // Loading Screen
  spr.fillSprite(TFT_BLACK);
  spr.setTextColor(BRAND_BLUE);
  spr.setTextFont(4);
  spr.setTextDatum(MC_DATUM);
  spr.drawString("DESK", 64, 50);
  spr.drawString("BUDDY", 64, 75);
  spr.setTextFont(2);
  spr.drawString("STARTING", 64, 105);
  spr.pushSprite(0, 0); 

  // Load Saved Preferences
  preferences.begin("clock_prefs", true);
  String ssid = preferences.getString("ssid", "");
  String pass = preferences.getString("password", "");
  use24HourFormat = (preferences.getString("time_format", "12") == "24");
  preferences.end();

  // AP Config for Captive Portal
  WiFi.mode(WIFI_AP_STA);
  IPAddress apIP(192, 168, 4, 1);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("Desk_Buddy", "C3_Zero@1234");
  dnsServer.start(53, "*", apIP); 

  if (ssid != "") {
    WiFi.begin(ssid.c_str(), pass.c_str());
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) { 
      delay(500); 
      attempts++; 
    }
  }

  // OTA Updates
  ArduinoOTA.setHostname("DeskBuddy-OTA");
  ArduinoOTA.setPassword("admin123");
  ArduinoOTA.begin();

  if (WiFi.status() != WL_CONNECTED) {
    isConfigMode = true; 
  } else {
    isConfigMode = false; 
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    fetchWeather(); 
  }

  // --- Web Server Setup ---
  server.on("/", HTTP_GET, [ssid]() {
    String html = dashboard_html;
    html.replace("__SAVED_SSID__", ssid);
    if(ssid != "") {
        html.replace("__SSID_READONLY__", "readonly style='opacity: 0.6; pointer-events: none;'");
        html.replace("__UNLOCK_BTN_DISPLAY__", "inline-block");
    } else {
        html.replace("__SSID_READONLY__", "");
        html.replace("__UNLOCK_BTN_DISPLAY__", "none");
    }
    server.send(200, "text/html", html);
  });
  
  server.on("/scan", HTTP_GET, []() {
    int n = WiFi.scanNetworks(); 
    String json = "[";
    for (int i = 0; i < n; ++i) { 
      if (i > 0) json += ","; 
      json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}"; 
    }
    json += "]"; 
    WiFi.scanDelete(); 
    server.send(200, "application/json", json);
  });

  server.on("/api/data", HTTP_GET, []() {
    String json = "{\"cpu\":" + String(currentCpuFreq) + ",\"temp\":" + String(currentChipTemp, 1) + ",\"rssi\":" + String(WiFi.RSSI()) + "}";
    server.send(200, "application/json", json);
  });

  server.on("/save-settings", HTTP_POST, []() {
    preferences.begin("clock_prefs", false);
    if(server.hasArg("ssid")) preferences.putString("ssid", server.arg("ssid"));
    if(server.hasArg("password") && server.arg("password").length() > 0) preferences.putString("password", server.arg("password"));
    if(server.hasArg("time_format")) preferences.putString("time_format", server.arg("time_format"));
    if(server.hasArg("lat")) preferences.putString("lat", server.arg("lat"));
    if(server.hasArg("lon")) preferences.putString("lon", server.arg("lon"));
    if(server.hasArg("city")) preferences.putString("city", server.arg("city"));
    preferences.end();
    server.send(200, "text/plain", "OK");
    delay(1000); ESP.restart();
  });

  server.on("/reboot", HTTP_POST, []() {
    server.send(200, "text/plain", "Rebooting");
    delay(1000); ESP.restart(); 
  });

  server.onNotFound([]() {
    server.sendHeader("Location", "http://192.168.4.1/", true);
    server.send(302, "text/plain", "");
  });

  server.begin();
}

void loop() { 
  unsigned long currentMillis = millis();

  // 1. Process Network Clients
  dnsServer.processNextRequest();
  server.handleClient(); 
  ArduinoOTA.handle(); 

  // 2. Sensor Polling & Throttling (Every 5 seconds)
  if (currentMillis - lastSlowTick > 5000) {
    lastSlowTick = currentMillis;
    currentChipTemp = temperatureRead();
    currentCpuFreq = getCpuFrequencyMhz();

    inTemp = dht.readTemperature();
    inHum = dht.readHumidity();

    if (currentChipTemp >= 70.0) {
        Serial.println("CRITICAL: 70C EXCEEDED. SLEEP.");
        goToSleep(); 
    }
    // Throttle step down to 80MHz on C3
    else if (currentChipTemp > 65.0 && currentCpuFreq == 160) { 
        setCpuFrequencyMhz(80); 
        isThrottled = true; 
    } 
    else if (currentChipTemp < 52.0 && currentCpuFreq == 80) { 
        setCpuFrequencyMhz(160); 
        isThrottled = false; 
    }

    // Weather Polling (Every 3 mins)
    weatherCounter++;
    if (weatherCounter >= 36 && !isConfigMode) { 
        fetchWeather(); 
        weatherCounter = 0; 
    }
  }

  // 3. Touch Pad Logic
  bool ttp1_isPressed = digitalRead(TOUCH_1) == HIGH;
  if (ttp1_isPressed && !ttp1_wasPressed) { 
    ttp1_pressTime = currentMillis; 
    ttp1_wasPressed = true;
  } else if (!ttp1_isPressed && ttp1_wasPressed) { 
    ttp1_wasPressed = false;
    long pressDuration = currentMillis - ttp1_pressTime;
    
    if (pressDuration > 600) {
      goToSleep(); 
    } 
    else if (pressDuration > 50) { 
      if (currentUI == 0) currentUI = 1;
      else if (currentUI == 1) currentUI = 3;
      else if (currentUI == 3) currentUI = 2;
      else if (currentUI == 2) currentUI = 0;
      
      uiTimer = currentMillis; 
    }
  }

  // Auto return to Main UI if viewing Weather (1) or Forecast (3) for > 8 seconds
  if ((currentUI == 1 || currentUI == 3) && (currentMillis - uiTimer > 8000)) {
      currentUI = 0;
  }

  // 4. Draw UI
  if (currentUI == 0) drawMainUI(); 
  else if (currentUI == 1) drawNestWeatherUI();
  else if (currentUI == 2) drawRobotEyes();
  else if (currentUI == 3) drawForecastUI();

  // Yield to RTOS Watchdog
  delay(isThrottled ? 33 : 16); 
}
