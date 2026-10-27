# Desk Buddy Firmware v2 — independent README-based rebuild

This is a clean implementation of the original repository README. The existing root sketch is **not read, modified, or replaced**. New firmware lives at \`firmware/DeskBuddyRebuilt/DeskBuddyRebuilt.ino\`.

## Build

From the repo root: \`pio run -c firmware/platformio.ini -d firmware/DeskBuddyRebuilt\` may not pick up the project source path because PlatformIO normally expects \`src/\`. The recommended approach is to run from \`firmware/\` with \`src_dir\` configured below, or copy \`DeskBuddyRebuilt.ino\` into a matching Arduino sketch folder.

**Arduino IDE**: Open \`firmware/DeskBuddyRebuilt/DeskBuddyRebuilt.ino\`. Select an ESP32-S3 board with Arduino-ESP32 3.x and install TFT_eSPI, DHT sensor library, Adafruit Unified Sensor, and ArduinoJson 7. Configure TFT_eSPI ST7735 to SPI MOSI 11, SCLK 12, CS 10, DC 9, RST 8, 128×160.

**PlatformIO**: Open the \`firmware\` folder and execute \`pio run\`. Uses the PlatformIO project configuration with explicit TFT pin definitions.

## Hardware

TFT: 11 MOSI, 12 CLK, 10 CS, 9 DC, 8 RST. MAX98357A: 6 BCLK, 7 LRCLK, 5 DIN, 14 SD. DHT11: 4 DATA. TTP223 active-high: 3 OUT. Status LED: 2. Common ground required.

## Important limitations and validation

- Board-specific verification on the actual ESP32-S3-Zero and 128×160 ST7735 display is still required. Some ST7735 panels need a different tab or pixel offset configuration; a GC9A01 must use a different display driver, and most GC9A01 units are not 128×160.
- MAX98357A expects a stable power supply. Do not power a 5V amplifier from a 3.3V pin; connect grounds together.
- TTP223 must be active-high (momentary) for the touch gestures and EXT1 touch wake described in the README. Check its quiescent output is LOW. Avoid leaving touch active when entering sleep.
- The temperature safety thresholds monitor the **on-chip** temperature, not the DHT11 ambient temperature. Deep sleep at >=70 C can itself require a touch wake while hot; deploy thermal protection only after bench testing.
- HTTPS weather requests currently use \`WiFiClientSecure::setInsecure()\` (encrypted but no certificate authentication). For production security replace with certificate-verified connections using an appropriate CA bundle.
- The basic note sequences are short original approximations, not complete copyrighted themes.
- Open-Meteo geocoding and forecast are refreshed approximately every 30 minutes when online; HTTP fetches are synchronous on Core 1 so web/UI can briefly pause during network timeouts. Audio stays on Core 0.
- The current implementation synchronizes time using the saved location string as the timezone; verify DST and timezone behavior for your deployment. Clock will not correctly ring until NTP sync succeeds.
- The status LED is held high during deep sleep, which continues drawing LED current. It is **not** an ultra-low-power sleep design if the LED is directly driven.
- Captive portal SSID/password are fixed exactly as specified in the README; configuration is saved in NVS. The password is readable by anyone who knows the published default; use this in a trusted environment.
- ArduinoOTA should be protected with a password before exposing the clock to an untrusted LAN.
- This branch has not been physically flashed or bench-tested.

## Touch actions

Tap: next screen or stop alarm. Double-tap: reboot into setup portal or snooze 5 minutes. Hold >650ms: sleep or dismiss today's ringing alarm.

## Web setup

Connect to \`Desk_Buddy\`, password \`Desk_Buddy1234@\`, and browse \`http://192.168.4.1\`. Choose Wi-Fi, city, 12/24-hour clock, accent color and up to ten alarms. Save to reboot.
