# 🖥️ Desk Buddy — Smart Desk Clock Firmware



![License](https://img.shields.io/badge/License-MIT-yellow.svg)![Status](https://img.shields.io/badge/Status-Active-brightgreen)![Audio](https://img.shields.io/badge/Audio-I2S-orange)



A dual-core ESP32-S3-Zero desk companion with a TFT display, weather forecasting, environmental sensing, a configurable multi-tone alarm system, and a captive-portal web dashboard for setup — all without needing a phone app.

---

## 📋 Table of Contents

- [Overview](#overview)
- [Hardware Requirements](#hardware-requirements)
- [Pin Mapping](#pin-mapping)
- [Architecture](#architecture)
- [Features](#features)
- [TFT Screens](#tft-screens)
- [Touch Controls](#touch-controls)
- [Web Dashboard](#web-dashboard)
- [Alarm System](#alarm-system)
- [Alarm Tones](#alarm-tones)
- [Setup & Installation](#setup--installation)
- [Required Libraries](#required-libraries)
- [First Boot / Wi-Fi Setup](#first-boot--wi-fi-setup)
- [OTA Updates](#ota-updates)
- [Thermal Management](#thermal-management)
- [Deep Sleep](#deep-sleep)
- [File Structure](#file-structure)
- [Customization](#customization)
- [Troubleshooting](#troubleshooting)
- [License](#license)

---

## Overview

**Desk Buddy** is a self-contained smart clock built for the **ESP32-S3-Zero**. It displays time, indoor temperature/humidity, live weather, a multi-day forecast, and a list of configured alarms — all on a small TFT screen, cycled via single-tap touch input. Configuration (Wi-Fi, location, alarms, appearance) is done entirely through a built-in web dashboard served by the device itself, accessible either on your home network or via its own captive-portal access point.

**Key philosophy:** the UI, sensors, and web server run on **Core 1**, while alarm audio playback is offloaded to a dedicated **FreeRTOS task pinned to Core 0**. This keeps the screen animations and touch responsiveness smooth even while a multi-note alarm melody is actively playing through the I2S DAC.

---

## Hardware Requirements

| Component | Notes |
|---|---|
| **ESP32-S3-Zero** | Dual-core, main controller — this firmware targets the S3-Zero specifically |
| TFT Display (ST7735/GC9A01-class, SPI) | 128×160 resolution, driven via TFT_eSPI |
| MAX98357A I2S Amplifier | Drives a small speaker for alarm tones |
| DHT11 | Indoor temperature & humidity sensor |
| Capacitive/touch pad | Single touch input (TTP223-style) |
| Status LED | Used for sleep/wake indication |
| Speaker | 4–8Ω, paired with MAX98357A |

---

## Pin Mapping

### TFT Display (FSPI)
| Signal | GPIO |
|---|---|
| MOSI | 11 |
| SCK  | 12 |
| CS   | 10 |
| DC   | 9  |
| RST  | 8  |

### I2S Audio (MAX98357A)
| Signal | GPIO |
|---|---|
| BCLK | 6 |
| LRC  | 7 |
| DIN  | 5 |
| SD (amp shutdown, HIGH = on) | 14 |

### Sensors & Controls
| Signal | GPIO |
|---|---|
| DHT11 Data | 4 |
| Touch Pad | 3 |
| Status LED | 2 |

> ⚠️ DHT11 VCC is expected to be hardwired directly to 3.3V (no switched power pin used) to conserve GPIOs.
> ⚠️ Pin numbers above are specific to the **ESP32-S3-Zero**'s GPIO layout — remap them if porting to a different S3 board.

---

## Architecture
