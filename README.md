# Desk Buddy

![Hardware](https://img.shields.io/badge/Hardware-ESP32--S3--Zero-1f6feb)
![License](https://img.shields.io/badge/License-MIT-yellow)
![Status](https://img.shields.io/badge/Status-In%20Development-blue)

**Desk Buddy** is a compact smart desk companion by **Robonavigators**, designed around the ESP32-S3-Zero. It combines a customizable clock, room environment monitoring, weather information, alarms, and an animated TFT interface in one small device.

The project aims to work independently on your desk, with optional smart-home connectivity through Home Assistant.

## Highlights

| Feature | Description |
| --- | --- |
| **Five display screens** | Clock, weather, three-day forecast, alarm list, and animated robot eyes |
| **Weather** | Current conditions and three-day outlook using OpenWeatherMap, with Open-Meteo as a fallback |
| **Room monitoring** | DHT11 temperature and humidity readings |
| **Alarms** | Ten configurable alarms, selectable tones, snooze, and dismiss controls |
| **Touch interaction** | Tap, double-tap, and long-press actions |
| **Wi-Fi setup** | A built-in captive portal for wireless configuration |
| **Interface** | Custom accent color, 12/24-hour clock, and planned smoother TFT animation |
| **Wireless updates** | OTA update support in the firmware design |

These features describe the **project design and ongoing beta development**; they should not be taken as a claim that every feature has passed hardware testing.

## Hardware

The current design uses the following components:

- **Microcontroller:** ESP32-S3-Zero
- **Display:** 128 × 160 SPI TFT (ST7735)
- **Audio:** MAX98357A I2S amplifier with a 4 Ω or 8 Ω speaker
- **Environment sensor:** DHT11
- **Input:** TTP223 capacitive touch sensor
- **Indicator:** GPIO-driven status LED
- **Power:** USB power, with suitable regulated supplies for the connected modules

### Pin connections

| Component | Signal | ESP32-S3 GPIO |
| --- | --- | --- |
| TFT | MOSI / SCK / CS / DC / RST | 11 / 12 / 10 / 9 / 8 |
| MAX98357A | BCLK / LRC / DIN / SD | 6 / 7 / 5 / 14 |
| DHT11 | Data | 4 |
| TTP223 | Output | 3 |
| Status LED | Control | 2 |
| TFT backlight | LED / BL | Depends on display module and backlight driver |

All connected modules must share a common ground. TFT controllers and backlight circuits vary between modules, so confirm the display's pinout and voltage requirements before wiring.

## Controls

| Gesture | Normal operation | While an alarm is ringing |
| --- | --- | --- |
| Single tap | Move to the next screen | Stop the alarm |
| Double tap | Open Wi-Fi setup | Snooze for five minutes |
| Long press | Enter deep sleep | Dismiss the alarm for the day |

## Wi-Fi and weather

The planned setup network uses the format **`Desk_Buddy-XXXX`**, where the four-character suffix distinguishes devices.

The captive portal is designed to allow configuration of Wi-Fi credentials, city, clock format, accent color, alarms, and an optional OpenWeatherMap API key. The weather system is designed to retrieve current conditions and a three-day forecast, falling back to Open-Meteo when necessary.

## Home Assistant integration

**Home Assistant support is being developed** using **mDNS** and **MQTT Discovery**.

- **mDNS:** a friendly local hostname, such as `desk-buddy-a3f2.local`, instead of relying on a changing IP address.
- **MQTT:** automatic discovery of Desk Buddy as a device in Home Assistant through a compatible MQTT broker, such as Mosquitto.
- **Monitoring:** room temperature, humidity, and alarm status.
- **Controls:** display screen selection, alarm enable/disable, and optional backlight brightness.

The planned MQTT setup uses a broker address, port, and optional credentials supplied through the Desk Buddy setup portal. The clock is intended to remain functional even when the broker is unavailable.

*Integration status: in development. MQTT discovery, network behavior, and Home Assistant controls have not yet been verified on the finished hardware.*

## Backlight brightness

PWM backlight dimming is being explored for TFT modules that expose a suitable backlight power input. A **BC558 PNP transistor** may be usable with a low-current **3.3 V** backlight, subject to verification of the module's circuit and current draw.

Backlight control is **not universal across TFT modules**. Some boards already have a suitable PWM input; others need a different driver or a logic-level MOSFET. This feature remains hardware-dependent and is not currently listed as tested.

## Project status

Desk Buddy is an **in-development hardware project**. Features, wiring, and the user interface may change as testing progresses. Firmware source and binaries are not currently distributed in this repository.

## About Robonavigators

**Robonavigators** builds robotics, embedded systems, IoT devices, and open-source technology projects.

## License

This repository is licensed under the [MIT License](LICENSE).
