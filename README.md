# Desk Buddy

![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg) ![Hardware: ESP32-S3-Zero](https://img.shields.io/badge/Hardware-ESP32--S3--Zero-blue.svg) ![Development: Hardware Prototype](https://img.shields.io/badge/Development-Hardware%20Prototype-blue.svg)

Desk Buddy is a Robonavigators ESP32-S3-Zero desk companion project: a 128×160 TFT desk clock with environment sensing, alarm audio, Wi-Fi setup and weather forecasting.

> **Source distribution policy — 6 November 2026:** The default `main` branch no longer hosts the original `Desk_Buddy.ino`, and the experimental rebuild files were removed from the beta branch. **The historical `rebuild/readme-firmware-v2` branch still contains an inherited copy of `Desk_Buddy.ino` at its tip; that copy has not yet been removed.** Firmware is shared separately, outside GitHub. Old commits and pull-request diffs may still contain historical versions; deleting current files does **not** erase Git history. The design specifications below are not a claim that a current downloadable firmware build exists.

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

## Home Assistant via mDNS and MQTT (planned beta integration)

> **Development status:** This describes the proposed local Arduino firmware changes, **not a tested or published release**. The code is shared in chat, not uploaded to GitHub.

Desk Buddy uses **mDNS** for a stable LAN address and **MQTT Discovery** for automatic Home Assistant entities. The ESP32 should remain fully functional as a standalone clock when Home Assistant or the MQTT broker is unavailable.

- **Device ID:** a stable four-character MAC suffix, e.g. `A3F2`.
- **Local hostname:** `desk-buddy-a3f2.local`; the mDNS HTTP service points to an optional local status page on port 80. mDNS depends on LAN multicast support; guest networks or isolated VLANs may block it.
- **Setup portal:** additionally asks for MQTT broker hostname/IP, TCP port (default `1883`), MQTT username/password, and optionally an OTA password. MQTT and Wi-Fi secrets are stored in ESP32 NVS, not displayed back in the form.
- **MQTT Discovery:** the MQTT integration in Home Assistant needs discovery enabled (default prefix `homeassistant`). A retained configuration is announced for each supported entity. An MQTT last will publishes `offline` if the connection drops.
- **Suggested entities:** DHT11 temperature, DHT11 humidity, TFT screen selection, master alarm enabled switch, ringing status, and **a backlight brightness light entity only when an appropriately wired PWM output is explicitly enabled**.
- **Connectivity:** connect to an MQTT broker on the same trusted LAN, for example a Home Assistant Mosquitto installation; reconnect automatically if the broker restarts. MQTT transport on port 1883 is **not encrypted**; restrict broker access, require credentials and avoid exposure to the Internet.
- **PWM hardware:** the BC558 wiring below remains a conditional proposal. With a compatible 3.3 V backlight supply, the PNP is inverted (0 duty at the transistor base means fully on). Never claim functional brightness control before electrical testing.

### Example MQTT topic layout

For a device with ID `a3f2`, the examples are:

| Purpose | Topic |
| --- | --- |
| Availability | `deskbuddy/a3f2/availability` |
| DHT11 temperature | `deskbuddy/a3f2/temperature/state` |
| DHT11 humidity | `deskbuddy/a3f2/humidity/state` |
| Screen select | `deskbuddy/a3f2/screen/set`, `deskbuddy/a3f2/screen/state` |
| Alarm master switch | `deskbuddy/a3f2/alarms/set`, `deskbuddy/a3f2/alarms/state` |
| Alarm ringing | `deskbuddy/a3f2/ringing/state` |
| Backlight | `deskbuddy/a3f2/backlight/set`, `deskbuddy/a3f2/backlight/state` |

### Home Assistant setup

1. Add a broker such as Mosquitto and enable Home Assistant's **MQTT integration**.
2. In the Desk Buddy captive portal, enter the broker's IP/hostname, port and MQTT credentials. A broker IP is usually more dependable across mDNS/VLAN configurations.
3. Save and reconnect the ESP32 to your Wi-Fi. Once MQTT connects, retained discovery records create device entities under **Settings → Devices & services → MQTT**.
4. Test temperature and screen switching first. Enable BC558/PWM only after verifying that your TFT has a suitable backlight supply input and current draw.

Documentation must be updated when this feature is compiled and tested; the present description must not be mistaken for a hardware-verified build.

## Development and documentation

The `main` branch intentionally hosts documentation, **not firmware source or binaries**. The older beta branch still has an inherited firmware file pending removal. New feature requests, bug fixes, hardware wiring decisions and test outcomes should be reflected here without uploading firmware. Historical firmware may remain accessible in Git commit history until removed separately.

## License

Project licensing: MIT; see [LICENSE](LICENSE).
