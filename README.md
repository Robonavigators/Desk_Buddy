# Desk Buddy <svg width="32" height="32" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 8a2 2 0 0 0-2 2v2a2 2 0 0 0 2 2h0a2 2 0 0 0 2-2v-2a2 2 0 0 0-2-2z"></path><path d="M7 10h10"></path><path d="M12 14v2"></path><rect x="5" y="6" width="14" height="12" rx="2"></rect><circle cx="9" cy="10" r="1"></circle><circle cx="15" cy="10" r="1"></circle></svg>

Desk Buddy is an ultra-low-power, switchless, battery-powered desktop companion designed for the ESP32-C3-Zero. It functions as a smart clock, weather station, and interactive digital pet, all while sipping minimal power to stay awake for months.

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

## 🔌 Pinout Configuration

Ensure your wiring matches the following mapping for the **Waveshare ESP32-C3-Zero**:

| Function | ESP32-C3 Pin | Connected Component |
| :--- | :--- | :--- |
| **Touch Input** | GPIO 3 | TTP223 Sensor |
| **DHT Power** | GPIO 4 | DHT11 VCC |
| **DHT Data** | GPIO 5 | DHT11 Data |
| **Sleep LED** | GPIO 8 | Status LED |
| **SPI MOSI** | GPIO 10 | TFT MOSI |
| **SPI MISO** | GPIO 9 | TFT MISO |
| **SPI SCK** | GPIO 8 | TFT SCK |

*Note: The DHT11 is power-gated via GPIO 4 to maximize battery life.*

## 🚀 Getting Started

1. **Wiring:** Wire according to the table above.
2. **Flash Firmware:** - Visit the [Robonavigators ESP Flasher](https://robonavigators.github.io/flash.html).
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
