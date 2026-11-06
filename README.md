# Desk Buddy

![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg) ![Hardware: ESP32-S3-Zero](https://img.shields.io/badge/Hardware-ESP32--S3--Zero-blue.svg) ![Development: Hardware Prototype](https://img.shields.io/badge/Development-Hardware%20Prototype-blue.svg)

Desk Buddy is a Robonavigators ESP32-S3-Zero desk companion project: a 128×160 TFT desk clock with environment sensing, alarm audio, Wi-Fi setup and weather forecasting.

> **Source distribution policy — 6 November 2026:** The repository no longer hosts the original `Desk_Buddy.ino` or the experimental rebuild files. Firmware is shared separately, outside GitHub. Old commits and pull-request diffs may still contain historical versions; deleting current files does **not** erase Git history. The design specifications below are not a claim that a current downloadable firmware build exists.

## Planned / previously prototyped features

- Five TFT screens: main clock, current weather, three-day forecast, ten alarms, robot eyes.
- Dual-core I2S alarm audio through MAX98357A and speaker.
- Touch control: tap to cycle/stop alarm, double tap for setup/snooze, hold for sleep/dismiss.
- Glassmorphism captive portal for Wi-Fi, city, clock format, accent and alarm settings.
- Proposed hotspot name `Desk_Buddy-XXXX` (stable four-character hardware suffix), password `Desk_Buddy1234@` and setup address `http://192.168.4.1`.
- Optional OpenWeatherMap key entered in setup; current weather and a three-day forecast using its three-hourly data, with Open-Meteo backup.
- Proposed buffered TFT frame updates to reduce flicker; reset font settings per frame and animate eyes smoothly with blinks.
- OTA, DHT11 ambient temperature/humidity, CPU thermal protection and deep-sleep touch wake.

**Important:** These items describe the project design and beta work discussed during development. Because firmware files have been removed, this repository does not verify that any particular firmware release implements them.

## Hardware

| Component | Connection | ESP32-S3-Zero |
| --- | --- | --- |
| ST7735 128×160 TFT | MOSI / SCK / CS / DC / RST | GPIO 11 / 12 / 10 / 9 / 8 |
| MAX98357A | BCLK / LRC / DIN / SD | GPIO 6 / 7 / 5 / 14 |
| DHT11 | DATA | GPIO 4 |
| TTP223 touch pad | OUT (active HIGH) | GPIO 3 |
| Status LED | control | GPIO 2 |
| TFT backlight | LED / BL | Module-dependent, see below |

TFT SPI configuration must match the **actual** controller and ST7735 tab variant. GC9A01 is not interchangeable with a 128×160 ST7735. Use a shared ground and power the MAX98357A according to its module specifications.

## BC558 backlight brightness — proposed hardware option

A **BC558 PNP** transistor can PWM-control a **3.3V, low-current TFT backlight** by switching the high side **if** the TFT exposes a backlight power input (LED/BL), rather than only a logic-enable input.

Suggested topology (subject to confirming your display's backlight circuit):

- BC558 emitter → **3.3V**, collector → TFT **backlight supply input (LED/BL)**; TFT GND stays connected as usual.
- One spare ESP32 output, e.g. GPIO 1 **if free and broken out**, → a base resistor (approximately 1 kΩ for light loads) → BC558 base.
- A **10 kΩ pull-up from base to emitter** keeps the transistor off during boot.
- PNP high-side PWM is inverted: GPIO LOW turns the backlight on; GPIO HIGH turns it off. Arduino-ESP32 3.x supports `ledcAttach(pin, freq, resolution)` and `ledcWrite(pin, duty)`.

**Safety limits:** The BC558 collector current absolute maximum is **100 mA**; stay comfortably below it and confirm the TFT's actual LED current. Keep the display's existing current-limiting resistor/driver. **Do not connect the BC558 emitter to 5V and drive its base directly from a 3.3V GPIO**—the output may not switch fully off. Some TFT boards already include a BL driver or tie the LED to VCC; inspect that module before rewiring. A correctly rated logic-level MOSFET is usually better for higher currents.

This is a hardware proposal only; no BC558/PWM control firmware has been published or tested.

## Development and documentation

This repository intentionally hosts project documentation, **not firmware source or binaries**. New feature requests, bug fixes, hardware wiring decisions and test outcomes should be reflected here without uploading firmware. Historical firmware may remain accessible in Git commit history until removed separately.

## License

Project licensing: MIT; see [LICENSE](LICENSE).
