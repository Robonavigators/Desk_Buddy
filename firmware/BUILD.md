# Desk Buddy — Beta rebuild build guide

The firmware at [`DeskBuddyRebuilt/DeskBuddyRebuilt.ino`](DeskBuddyRebuilt/DeskBuddyRebuilt.ino) is a separate, untested implementation based on the project README. The original root sketch was **not opened, edited or replaced**.

## Building

**Arduino IDE:** Open `firmware/DeskBuddyRebuilt/DeskBuddyRebuilt.ino`, choose ESP32-S3 (Arduino-ESP32 **3.x**) and install `TFT_eSPI`, `ArduinoJson 7`, `DHT sensor library` and `Adafruit Unified Sensor`. Update **TFT_eSPI User_Setup** for the actual panel before compiling; sketch-local pin macros will not automatically reconfigure separately compiled library source.

**PlatformIO:** Open the `firmware` folder and run `pio run`. Its `platformio.ini` sets the source directory and the initial ST7735 TFT settings. Validate the exact ESP32-S3-Zero flash/partition configuration and display tab variant before flashing.

TFT: ST7735 128x160, MOSI GPIO 11, SCLK 12, CS 10, DC 9, RST 8. MAX98357A: BCLK 6, WS 7, DIN 5, SD 14. DHT11: data 4. TTP223 active-HIGH output: 3. Status LED: 2. Use a common ground and power the amplifier as required by its hardware.

## Feature and display notes

- Setup access point: `Desk_Buddy-XXXX` (last 4 hexadecimal characters based on the ESP32 MAC); password: `Desk_Buddy1234@`; portal: `http://192.168.4.1`.
- The portal saves Wi-Fi, city, optional OpenWeatherMap key, 12/24-hour clock, accent color, and 10 alarms in NVS.
- OpenWeatherMap provides current weather and 3-day approximate high/low temperatures from a 3-hourly forecast when the key is configured. Open-Meteo is the fallback when a key is missing or requests fail. Requests refresh around every 30 minutes.
- Display pages: Clock, Weather, 3-Day Forecast, Alarms, Robot Eyes. Each frame is composed in a `TFT_eSprite` (16-bit, 8-bit allocation fallback) then pushed in one update to reduce flicker. Font 1/size 1 is reset at every frame. Eye animation is sine-driven with blinks, ~10 fps; other pages update about once a second.
- The ArduinoJson 7 compatibility fix uses `.as<JsonArray>().size() > 0` rather than `.isEmpty()`.

## Limitations and testing checklist

- **Not yet compile-verified or flashed** on actual ESP32-S3-Zero hardware. Verify exact TFT ST7735 driver tab/offset and color order; GC9A01 displays require another build configuration.
- HTTPS currently uses `WiFiClientSecure::setInsecure()` (no server certificate verification). **Do not deploy as secure** until certificate verification is configured.
- ArduinoOTA lacks authentication; run only on a trusted LAN until it is protected.
- Wi-Fi/AP fixed published password is meant only for hobby setup, not a security boundary. The OpenWeatherMap API key is saved in ESP32 NVS; it is not displayed back in the portal.
- Weather calls are synchronous on Core 1 and may briefly block the UI, even though I2S alarm audio runs on Core 0.
- Thermal thresholds read ESP32 chip temperature, not ambient DHT11. Deep sleep with LED held HIGH still consumes current and **does not wake for scheduled alarms**.
- Timezone uses a stored UTC offset sourced from the provider. Daylight-saving transitions require re-sync; test for regions with DST.
- TTP223 must be active-HIGH momentary for EXT1 touch wake. Check touch is released before sleeping.
- Melody sequences are short illustrative approximations, not full theme recordings.
- Whenever modifying firmware, update `README.md` in the same change to document features, pin/wiring changes, fixes, test status, and any limitations.

## Touch gestures

Single tap cycles screens or stops a ringing alarm. Double tap enters setup or snoozes an alarm for five minutes. Long press (>650 ms) enters deep sleep or dismisses ringing alarm for the day.
