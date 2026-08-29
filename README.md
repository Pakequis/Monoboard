## Important warning

**This project is experimental.** Sensor data collected (temperature, humidity, etc.) **are not highly accurate** and should be used only as reference, **not as critical data**.

# Monoboard

Firmware project for a dashboard on a Waveshare 7.5" e-paper display, driven by an ESP32-S3 DevKitC-1. There's no footer — a one-line header shows the title, WiFi status, and firmware version. Below it, a 2×2 content grid shows weather forecast + local temperature/humidity, a lightning-strike distance/rate box (AS3935), and a monthly calendar on top; a DSEG7 seven-segment clock (synced via NTP, with an empty box reserved below it for future content) and a rotating news-headline carousel on the bottom. Every display-facing string switches between PT-BR/EN at compile time (`include/strings.h`). The board wakes from deep sleep on a timer (`DEEP_SLEEP_INTERVAL_SEC`, currently `60`s — the confirmed production value) or the AS3935 lightning sensor's IRQ pin, redraws, and goes back to sleep.

## Functionalities

- **E‑paper display** – Waveshare 7.5″ (640 × 384 px, landscape). All content is refreshed via `updateScreen()` in paged mode.
- **Deep‑sleep power saving** – The board sleeps for `DEEP_SLEEP_INTERVAL_SEC` (currently 60 s) and wakes on timer or the AS3935 lightning sensor’s IRQ pin (`ext1`).
- **Weather forecast** – Pulls 6‑hour forecast from Open‑Meteo (temperature, weather codes, day/night icons). Labels and month/weekday names are displayed in Portuguese or English per `APP_LANGUAGE`.
- **Local sensors** – DHT22 (temperature / humidity).
- **Lightning detection** – SparkFun AS3935 detects lightning strikes, reports distance rings (10/20/30/40 km) and stores a 5‑entry history in RTC memory.
- **Monthly calendar** – Shows the current month with weekday/ month names translated; 
- **Seven‑segment clock** – DSEG7 Classic Bold font, synced via NTP; displays `HH:MM` in the bottom‑right quadrant.
- **News carousel** – Rotating carousel of headline headlines from Google RSS (PT‑BR or EN locale). Enabled/disabled via `FEATURE_NEWS_ENABLED`.
- **Compile‑time language switch** – Change `#define APP_LANGUAGE LANG_PT_BR` to `LANG_EN` in `config.h` (or pass `-D APP_LANGUAGE=LANG_EN`) to switch all UI strings without touching code.
- **String catalog** – All user‑facing strings live in `include/strings.h` with separate blocks for Portuguese and English (weekdays, months, labels, warnings, etc.).
- **Power management** – After screen update the ESP32‑S3 enters deep sleep drawing ~5‑10 µA; Wi‑Fi is only powered on when a sync window is due (NTP resync every hour).

## Docs Structure

| File | Description |
|---------|-----------|
| `docs/Pin Mapping.md`   | ESP32 and ESP32-S3 pin mapping for the display and sensors |
| `docs/API Reference.md` | Code usage guide — available functions and examples |
| `docs/Internationalization.md` | PT-BR/EN string-switching mechanism: keys, constraints, how to add strings/languages |

## Technologies

- **Platform**: PlatformIO
- **Framework**: Arduino (ESP32-S3; `esp32-s3-devkitc-1` is the production firmware env — see `platformio.ini`). A separate `native` environment (no board, no Arduino) runs host-compiled Unity unit tests for a few pure-logic modules — `pio test -e native`.
- **Display**: Waveshare 7.5" (GxEPD2_750) — 640×384 px
- **Libraries**: GxEPD2, Adafruit GFX, Adafruit BusIO, ArduinoJson, DHT sensor library, Adafruit Unified Sensor, SparkFun AS3935 Lightning Detector, WiFi, HTTPClient (all in `platformio.ini`'s `lib_deps` except the last two, which ship with the Arduino-ESP32 core)

## Hardware

Confirmed via `esptool flash_id`:

- **Chip**: ESP32-S3 (QFN56), revision v0.2, WiFi + BLE, 40MHz crystal
- **Flash**: 16MB (Winbond, quad SPI, 3.3V)
- **PSRAM**: 8MB embedded (octal) — enabled (`board_build.arduino.memory_type = qio_opi` + `-DBOARD_HAS_PSRAM` in `platformio.ini`) and confirmed working on real hardware. The full 16MB of flash is usable via `board_build.partitions = default_16MB.csv` + `board_upload.flash_size = 16MB` in `platformio.ini` — both keys are required, since the espressif32 build script sizes the flashed image header from `board_upload.flash_size` specifically, not `board_build.flash_size`; with only the latter set, the bootloader stays capped at the board's 8MB default regardless of the partition table.


