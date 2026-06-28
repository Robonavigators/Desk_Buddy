# Desk_Buddy 

Desk Buddy is an ultra-low-power, switchless, battery-powered desktop companion designed for the Waveshare ESP32-C3-Zero. It functions as a smart clock, weather station, and interactive digital pet, all while sipping minimal power to stay awake for months.

![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)
![Hardware: ESP32-C3](https://img.shields.io/badge/Hardware-ESP32--C3-orange.svg)
![Status: Stable](https://img.shields.io/badge/Status-Stable-brightgreen.svg)

## 📋 Features

* **Switchless Interface:** Completely solid-state design utilizing capacitive touch sensors.
* **Intelligent Power Management:**
    * Deep sleep standby (~12.5 µA).
    * Dynamic sensor power gating (DHT11 is only powered when reading).
    * Thermal throttling to prevent overheating.
* **4-in-1 UI Layers:**
    * **Clock:** High-contrast time, indoor environment, and weather summary.
    * **Today's Weather:** Graphical condition icons and humidity monitoring.
    * **3-Day Forecast:** Upcoming weather trends.
    * **Robot Eyes:** Animated personality mode with color-shifting pupils.
* **Web-Based Config:** Built-in Captive Portal for easy Wi-Fi, location, and timezone setup.

## 🏗 Hardware Assembly & Pinout

We use the "Dead Bug" point-to-point soldering method for an ultra-thin footprint.



| Function | ESP32-C3 Pin | Connected Component |
| :--- | :--- | :--- |
| **Touch Input** | GPIO 3 | TTP223 Sensor |
| **DHT Power** | GPIO 4 | DHT11 VCC |
| **DHT Data** | GPIO 5 | DHT11 Data |
| **Backlight/LED**| GPIO 8 | BC558 Base (via 1kΩ Resistor) |
| **SPI MOSI** | GPIO 10 | TFT MOSI |
| **SPI MISO** | GPIO 9 | TFT MISO |
| **SPI SCK** | GPIO 8 | TFT SCK |
| **TFT CS** | GPIO 6 | TFT CS |
| **TFT DC** | GPIO 7 | TFT DC |
| **TFT RST** | GPIO 2 | TFT RST |

*Note: The DHT11 is power-gated via GPIO 4 and the backlight via GPIO 8 (BC558 PNP high-side switch) to maximize battery life.*

## 🚀 Getting Started

1. **Wiring:** Wire according to the table above. Ensure a 1kΩ resistor is used between GPIO 8 and the BC558 Base.
2. **Flash Firmware:** - Visit the [RoboNavigators ESP Flasher](https://robonavigators.github.io/flash.html).
   - Select **"Desk_Buddy Firmware"** from the menu.
   - Connect your ESP32-C3-Zero via USB.
   - Select your COM/Serial port and click **Upload**.
3. **Setup:** On first boot, the device will broadcast an AP named `Desk_Buddy`. Connect to it and navigate to `192.168.4.1` to input your Wi-Fi credentials.

## 🛠 Hardware Used

| Component | Specification |
| :--- | :--- |
| **Brain** | Waveshare ESP32-C3-Zero |
| **Display** | 128x160 TFT LCD (ST7735/ST7789) |
| **Sensor** | DHT11 (Temperature/Humidity) |
| **Input** | TTP223 Capacitive Touch Module |
| **Battery** | Li-Po (with TP4056 USB-C Charger) |

## ⚖️ License

Distributed under the **MIT License**. See `LICENSE` for more information.
