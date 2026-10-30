# Desk Buddy

> **Beta development build — 30 October 2026.** The independent firmware rebuild lives in [`firmware/DeskBuddyRebuilt/DeskBuddyRebuilt.ino`](firmware/DeskBuddyRebuilt/DeskBuddyRebuilt.ino) on branch `rebuild/readme-firmware-v2`. The original root `Desk_Buddy.ino` remains unchanged. This beta has **not** been hardware-verified or compile-verified; do not mistake the website flasher's current firmware for this rebuild.




![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg). ![Status: Active](https://img.shields.io/badge/Status-Active-brightgreen.svg). ![Hardware: ESP32-S3-Zero](https://img.shields.io/badge/Hardware-ESP32--S3--Zero-blue.svg). ![Audio: I2S](https://img.shields.io/badge/Audio-I2S%20Digital-orange.svg)



A dual-core smart desk clock built on the ESP32-S3-Zero. This project combines a TFT display, live weather forecasting, environmental sensing, and a configurable multi-tone alarm system, all managed through a built-in Captive Portal web dashboard with a modern Glassmorphism UI.

It features a **Core-Split Audio Engine** that offloads alarm melody playback to its own dedicated processor core, so the clock face, touch input, and web server never stutter — even mid-alarm. Alongside this, the onboard Captive Portal allows you to configure Wi-Fi, location, appearance, and all 10 alarm slots on the fly.

## ✨ Features

* **Buffered TFT & Animated Eyes (Beta):** Uses an off-screen TFT_eSprite frame (16-bit, with 8-bit allocation fallback) to reduce visible redraw flicker. Resets the font/size on every frame. Robot-eye motion uses sine-based movement with blink animation, rendered approximately 10 times per second; other pages refresh roughly once per second.
* **Core-Split Audio Engine:** Alarm melodies are generated and streamed on Core 0, fully independent from the Core 1 UI/sensor/web-server loop, so playback never blocks the display and can be interrupted mid-note by a tap.
* **Multi-Tone Alarm System:** 10 independent alarm slots, each with its own time and selectable tone (Mario, Zelda, Beep, StarWars — easily expandable). All alarms are viewable at a glance on a dedicated TFT Alarms List screen.
* **Captive Portal Configuration:** Built-in DNS server and Glassmorphism setup UI for Wi-Fi, city, optional OpenWeatherMap API key, time format, accent color, and 10 alarms. Settings are stored in ESP32 non-volatile memory (NVS). The access point is named `Desk_Buddy-XXXX` (stable four hexadecimal characters derived from hardware ID).
* **Live Weather & Three-Day Forecast:** OpenWeatherMap is preferred for current conditions and three forecast days when its optional API key is saved in setup. The three-day high/low values are estimated from OpenWeatherMap's three-hour intervals (today may be partial). Open-Meteo provides a no-key fallback for current weather and/or forecast. City names are geocoded using Open-Meteo. Weather refresh is roughly every 30 minutes while connected.
* **Smart Touch Control:** Single tap cycles screens or stops a ringing alarm; double tap snoozes an alarm or opens Setup Mode; long press dismisses an alarm or triggers Deep Sleep.
* **Thermal Throttling:** Automatically underclocks the CPU to 80MHz if the core temperature exceeds 65°C, restoring full 240MHz performance once cooled to 52°C. Forces Deep Sleep as a safety cutoff at 70°C.
* **OTA Updates:** Wireless firmware updates over the local network once Wi-Fi is connected.
* **Deep Sleep with Wake-on-Touch:** Powers down the display and holds the status LED during sleep; any touch press wakes the device.

---

## 🛠️ Hardware Requirements

You will need the following components:

* **Microcontroller:** ESP32-S3-Zero
* **Display:** 128×160 SPI **ST7735 TFT** for the beta configuration. Other controllers such as GC9A01 require separate driver and geometry changes; they are **not** interchangeable.
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

### Beta source build (Arduino IDE or PlatformIO)

The beta sketch is at `firmware/DeskBuddyRebuilt/DeskBuddyRebuilt.ino`. Use **Arduino-ESP32 3.x** and install **TFT_eSPI**, **ArduinoJson 7**, **DHT sensor library**, and **Adafruit Unified Sensor**. For PlatformIO, open the `firmware` directory and run `pio run` using its `platformio.ini`.

**TFT_eSPI warning:** Arduino IDE compiles the TFT_eSPI library separately, so macros defined only inside the sketch do **not** reliably override the library's configured driver/pins. Configure the library's User_Setup / User_Setup_Select for ST7735, 128×160, MOSI 11, SCLK 12, CS 10, DC 9, RST 8 and a suitable ST7735 tab variant. The included PlatformIO build flags specify these settings. Wrong tab/offset settings may display shifted colors or misaligned content.

### Existing website flasher (original firmware)

These existing flasher instructions refer to the repository's published firmware; **the beta rebuild is not automatically uploaded by the flasher**.

The easiest way to install the published firmware is directly through your web browser using Web Serial:

1. Go to [Robonavigators ESP Flasher](https://robonavigators.github.io/flash.html).
2. Choose **Desk_Buddy Firmware** from the firmware menu.
3. Click the connect button and select your ESP32-S3-Zero's COM/Serial port from the browser popup.
4. Click upload to flash the firmware directly to your board.

---

## ⚙️ Network Configuration (Captive Portal)

On first boot, or whenever you need to change Wi-Fi credentials, location, or alarms, Desk Buddy hosts its own Wi-Fi network and pops up a configuration portal.

1. Power on the clock. (If Wi-Fi is already saved and you want to change it, **double-tap the touch pad** while idle to force it back into Setup Mode.)
2. On your phone or computer, connect to the new Wi-Fi network:
   * **SSID (beta):** `Desk_Buddy-XXXX` (for example `Desk_Buddy-A3F2`; suffix stays constant for each board)
   * **Password:** `Desk_Buddy1234@`
3. A login portal should automatically appear on your screen. (If it does not, open a web browser and navigate to `http://192.168.4.1`).
4. The blue (`#0484f8`) Glassmorphism portal lets you scan Wi-Fi, enter the Wi-Fi password, specify your city, optionally enter an **OpenWeatherMap API key**, select 12/24-hour format and accent color, and configure ten alarms. Leave the key blank to keep a saved key; choose **Clear saved API key** to remove it. The key is stored in NVS and not echoed into the setup form.
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

## 🖥️ Display rendering and fonts (beta)

- Resolution: **128×160**, ST7735 and TFT_eSPI; `setRotation(0)`. Screen order: Main → Weather → Forecast → Alarms → Robot Eyes.
- A **128×160 off-screen sprite** is assembled and pushed in one operation instead of clearing the physical panel every 350 milliseconds. 16-bit frames require approximately **40 KB** RAM; a smaller 8-bit frame is attempted if needed.
- The UI explicitly selects built-in font 1 and resets font size each frame. Set correct driver/tab/pin options in TFT_eSPI; changing sketch-level typography does not fix a wrong display driver.
- Eye animation uses smoothly drifting pupils and occasional blinks, with ~100 ms frame interval. Clock/weather/alarms update around once per second. Physical panel testing is still required to confirm visual alignment and smoothness.

## 🌦️ Weather providers (beta)

| Provider | Current conditions | 3-day forecast | API key |
| --- | --- | --- | --- |
| OpenWeatherMap | Preferred | Preferred, grouped from 3-hour steps | Optional key in setup |
| Open-Meteo | Fallback | Fallback daily highs/lows | None |

The forecast includes today and the next two local calendar dates when available; today's extrema may cover only the remaining forecast intervals. The firmware requests weather roughly every 30 minutes on an active Wi-Fi connection. Current weather values from the two APIs may differ. A failed provider request can briefly delay Core 1 UI/network processing; Core 0 audio remains separate.

## ⚠️ Beta limits and security

- This branch has **not been fully compile-tested or physically tested** on your ESP32-S3-Zero.
- The ST7735 tab/offset variant must match the actual module; otherwise colors, alignment or dimensions may be wrong.
- OpenWeatherMap and Open-Meteo HTTPS currently use `WiFiClientSecure::setInsecure()` (certificate verification disabled). Replace with trusted CA verification before production deployment.
- ArduinoOTA has no configured password yet. Use only on a trusted local network until protected.
- A common published AP password is convenient for setup but should not be treated as secure access control.
- The core temperature thresholds concern the ESP32's **internal chip sensor**, not ambient DHT11 temperature; verify thermal behavior on hardware.
- The status LED is held HIGH in deep sleep and can continue consuming power. The beta does not guarantee timed alarms wake the device from deep sleep.
- `README.md` must be updated **alongside every firmware feature or bug fix**, including build status, wiring, UX, and limitations. Do not describe untested behavior as verified.

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
