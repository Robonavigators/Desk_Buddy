# Desk Buddy



![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)  ![Status: Active](https://img.shields.io/badge/Status-Active-brightgreen.svg) ![Hardware: ESP32-S3-Zero](https://img.shields.io/badge/Hardware-ESP32--S3--Zero-blue.svg)




![Audio: I2S](https://img.shields.io/badge/Audio-I2S%20Digital-orange.svg)



A dual-core smart desk clock built on the ESP32-S3-Zero. This project combines a TFT display, live weather forecasting, environmental sensing, and a configurable multi-tone alarm system, all managed through a built-in Captive Portal web dashboard with a modern Glassmorphism UI.

It features a **Core-Split Audio Engine** that offloads alarm melody playback to its own dedicated processor core, so the clock face, touch input, and web server never stutter — even mid-alarm. Alongside this, the onboard Captive Portal allows you to configure Wi-Fi, location, appearance, and all 10 alarm slots on the fly.

## ✨ Features

* **Core-Split Audio Engine:** Alarm melodies are generated and streamed on Core 0, fully independent from the Core 1 UI/sensor/web-server loop, so playback never blocks the display and can be interrupted mid-note by a tap.
* **Multi-Tone Alarm System:** 10 independent alarm slots, each with its own time and selectable tone (Mario, Zelda, Beep, StarWars — easily expandable). All alarms are viewable at a glance on a dedicated TFT Alarms List screen.
* **Captive Portal Configuration:** Built-in DNS server and Access Point (Glass UI) to configure Wi-Fi, city/location, time format, accent color, and alarms, saved to non-volatile memory (NVS).
* **Live Weather & Forecast:** Auto-geocodes a typed city name via Open-Meteo (no API key required), displaying current conditions and a 3-day forecast.
* **Smart Touch Control:** Single tap cycles screens or stops a ringing alarm; double tap snoozes an alarm or opens Setup Mode; long press dismisses an alarm or triggers Deep Sleep.
* **Thermal Throttling:** Automatically underclocks the CPU to 80MHz if the core temperature exceeds 65°C, restoring full 240MHz performance once cooled to 52°C. Forces Deep Sleep as a safety cutoff at 70°C.
* **OTA Updates:** Wireless firmware updates over the local network once Wi-Fi is connected.
* **Deep Sleep with Wake-on-Touch:** Powers down the display and holds the status LED during sleep; any touch press wakes the device.

---

## 🛠️ Hardware Requirements

You will need the following components:

* **Microcontroller:** ESP32-S3-Zero
* **Display:** TFT panel (ST7735/GC9A01-class, SPI), 128×160
* **Amplifier:** MAX98357A I2S Class D Audio Amplifier
* **Speaker:** 4Ω or 8Ω speaker (1W - 3W)
* **Sensor:** DHT11 Temperature & Humidity sensor
* **Touch Sensor:** Capacitive touch pad (TTP223-style)
* **Status LED:** Any GPIO-driven LED
* **Power:** USB-C or regulated 3.3V supply

---

## 🔌 Wiring Diagram

To keep the pin count manageable, the TFT, I2S amplifier, DHT11, touch pad, and status LED each use dedicated GPIOs on the S3-Zero.

| Component | Pin | S3-Zero Pin (GPIO) | Notes |
| :--- | :--- | :--- | :--- |
| **TFT Display** | MOSI | GPIO 11 | FSPI |
| | SCK | GPIO 12 | FSPI |
| | CS | GPIO 10 | |
| | DC | GPIO 9 | |
| | RST | GPIO 8 | |
| **MAX98357A (Amp)** | VIN | 5V | 5V provides louder audio |
| | GND | GND | Ground |
| | BCLK | GPIO 6 | I2S Bit Clock |
| | LRC | GPIO 7 | I2S Word Select |
| | DIN | GPIO 5 | I2S Data **OUT** |
| | SD | GPIO 14 | Amp Shutdown (HIGH = on) |
| **DHT11** | Data | GPIO 4 | VCC hardwired to 3.3V |
| **Touch Pad** | Signal | GPIO 3 | Also used as Deep Sleep wake source |
| **Status LED** | Signal | GPIO 2 | Held HIGH during sleep |

*Wire the speaker directly to the + and - terminals on the MAX98357A board.*

---

## ⚡ Installation

1. Install the required libraries in Arduino IDE / PlatformIO: `WiFi`, `WebServer`, `DNSServer`, `Preferences`, `HTTPClient`, `ArduinoJson` (v7+), `TFT_eSPI`, `I2S`, `ArduinoOTA`, and the Adafruit `DHT sensor library`.
2. Edit `User_Setup.h` inside the `TFT_eSPI` library to match the pin mapping above and select the correct display driver. **This step is required** — the S3-Zero pinout is not TFT_eSPI's default target.
3. Select an **ESP32-S3-Zero** board profile in your IDE, matching your board's flash/PSRAM configuration.
4. Place `desk_buddy.ino` and `alarms.h` together in the same sketch folder.
5. Upload over USB.

---

## ⚙️ Network Configuration (Captive Portal)

On first boot, or whenever you need to change Wi-Fi credentials, location, or alarms, Desk Buddy hosts its own Wi-Fi network and pops up a configuration portal.

1. Power on the clock. (If Wi-Fi is already saved and you want to change it, **double-tap the touch pad** while idle to force it back into Setup Mode.)
2. On your phone or computer, connect to the new Wi-Fi network:
   * **SSID:** `Desk_Buddy`
   * **Password:** `Desk_Buddy1234@`
3. A login portal should automatically appear on your screen. (If it does not, open a web browser and navigate to `http://192.168.4.1`).
4. You will see a blue (`#0484f8`) screen with a Glassmorphism card. Scan and select your home Wi-Fi network, enter your password, set your city name, choose a time format and accent color, and configure your alarms with times and tones.
5. Click **Save All Settings**. The AP will turn off, and the device will reboot into normal clock mode, connecting to your network and syncing time/weather.

---

## 🚀 Usage Guide

* **Cycle Screens:** Single tap to move through Main → Weather → Forecast → Alarms List → Robot Eyes → Main.
* **Stop an Alarm:** Single tap while an alarm is ringing.
* **Snooze:** Double tap while an alarm is ringing (5-minute snooze).
* **Dismiss for the Day:** Long press (more than 600 milliseconds) while an alarm is ringing.
* **Setup Mode:** Double tap the touch pad while idle to reboot directly into the Wi-Fi Captive Portal for reconfiguration.
* **Deep Sleep:** Long press the touch pad while idle to power down the display and enter ultra-low-power mode. Press the button again to wake the device back up.

---

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
