## Important warning

**This project is experimental.** Sensor data collected (temperature, humidity, etc.) **are not highly accurate** and should be used only as reference, **not as critical data**.

# Monoboard

![Monoboard dashboard, assembled and running](images/monoboard-1.JPG)

I've had this Waveshare 7.5" e-paper display sitting since 2019, waiting for a project to use it for. This is that project, better late than never.

Dashboard firmware for the panel, driven by an ESP32-S3 DevKitC-1. Weather, local temperature/humidity, lightning strikes, a monthly calendar, a clock, crypto quotes and a news carousel. UI strings switch between PT-BR/EN at compile time. The board wakes from deep sleep on a timer or an AS3935 IRQ, redraws, and goes back to sleep.

## Functionalities

- **E‑paper display**: Waveshare 7.5″ (640 × 384 px, landscape), refreshed via `updateScreen()` in paged mode. Periodic black/white refresh cycles clear the ghosting the GDEW075T8 panel accumulates in static regions, tunable via `DISPLAY_CONDITION_*`. See `docs/API Reference.md`.
- **Weather & local sensors**: 6‑hour forecast from Open‑Meteo plus DHT22 temperature/humidity.
- **Lightning detection**: SparkFun AS3935 detects strikes with an adaptive distance scale, a strikes-per-hour count and a 5‑entry history. Tuned to `OUTDOOR` gain with tightened noise rejection. See `docs/Lightning Detection.md`.
- **Monthly calendar**, **seven‑segment clock** (DSEG7, NTP-synced), **crypto quotes** (BTC/ETH via CoinGecko) and a **news carousel** (Google RSS).
- **Screen measurement mode**: `SHOW_SCREEN_RULER` build flag draws calibrated centimetre rulers latched on the panel as a 1:1 physical template. See `docs/Screen Measurement Mode.md`.
- **Power management**: the board sleeps `DEEP_SLEEP_INTERVAL_SEC` (60 s) at ~5‑10 µA, woken by timer or the AS3935 IRQ (`ext1`). WiFi only powers on for the hourly NTP resync.
- **Language**: `APP_LANGUAGE` (`LANG_PT_BR`/`LANG_EN`) switches all UI strings (`include/strings.h`) at compile time, no code changes needed.

## Enclosure

The dashboard sits in a small pine wood frame built around the e-paper panel. Piece dimensions, drawings and assembly are in `docs/Enclosure Build.md`.

![Screen ruler pattern latched on the panel, used to fit the wood frame around it](images/wood-2.JPG)

![Wiring inside the assembled frame: driver HAT, DHT22 and ESP32-S3 header](images/inside-1.JPG)

## Docs Structure

| File | Description |
|---------|-----------|
| `docs/Pin Mapping.md`   | ESP32 and ESP32-S3 pin mapping for the display and sensors |
| `docs/API Reference.md` | Code usage guide: available functions and examples |
| `docs/Internationalization.md` | PT-BR/EN string-switching mechanism: keys, constraints, how to add strings/languages |
| `docs/Lightning Detection.md` | AS3935 IRQ wake, the strikes/hour metric, noise-rejection tuning, the `as3935_monitor` diagnostic build |
| `docs/Screen Measurement Mode.md` | `SHOW_SCREEN_RULER`: edge rulers + calibrated 10 cm bar latched on the panel as a physical template |
| `docs/Enclosure Build.md` | Pine wood frame around the e-paper panel: piece dimensions, drawings, groove assembly |

## Technologies

- **Platform**: PlatformIO
- **Framework**: Arduino (ESP32-S3). `esp32-s3-devkitc-1` is the production firmware env, see `platformio.ini`. A separate `native` environment (no board, no Arduino) runs host-compiled Unity unit tests for a few pure-logic modules, see `pio test -e native`.
- **Extra build environments** (`platformio.ini`): `screen_ruler`, the production binary with `-DSHOW_SCREEN_RULER=1` (see `docs/Screen Measurement Mode.md`). `as3935_monitor` (plus `as3935_monitor_loose` / `as3935_monitor_outdoor` variants) and `wifi_lightning_noise_test`, standalone diagnostic sketches for the lightning sensor (see `docs/Lightning Detection.md`). None of these are flashed as the normal firmware.
- **Build flags**: `APP_DEBUG_SERIAL` (serial logging on/off, default off), `SHOW_SCREEN_RULER` (default off), `APP_LANGUAGE` (`LANG_PT_BR`/`LANG_EN`): all in `config.h` under `#ifndef`, so a `-D` flag overrides without editing the file.
- **Display**: Waveshare 7.5" (GxEPD2_750): 640×384 px
- **Libraries**: GxEPD2, Adafruit GFX, Adafruit BusIO, ArduinoJson, DHT sensor library, Adafruit Unified Sensor, SparkFun AS3935 Lightning Detector, WiFi, HTTPClient (all in `platformio.ini`'s `lib_deps` except the last two, which ship with the Arduino-ESP32 core)

## Hardware

Confirmed via `esptool flash_id`:

- **Chip**: ESP32-S3 (QFN56), revision v0.2, WiFi + BLE, 40MHz crystal
- **Flash**: 16MB (Winbond, quad SPI, 3.3V)
- **PSRAM**: 8MB embedded (octal), enabled (`board_build.arduino.memory_type = qio_opi` + `-DBOARD_HAS_PSRAM` in `platformio.ini`) and confirmed working on real hardware. The full 16MB of flash is usable via `board_build.partitions = default_16MB.csv` + `board_upload.flash_size = 16MB` in `platformio.ini`: both keys are required, since the espressif32 build script sizes the flashed image header from `board_upload.flash_size` specifically, not `board_build.flash_size`. With only the latter set, the bootloader stays capped at the board's 8MB default regardless of the partition table.

## Parts

Affiliate links (AliExpress): buying through them costs you nothing extra and gives me a small commission.

| Part | Link |
|---|---|
| ESP32-S3 DevKitC-1 | [Buy on AliExpress](https://s.click.aliexpress.com/e/_c3HgUNVP) |
| Waveshare 7.5" e-paper display | [Buy on AliExpress](https://s.click.aliexpress.com/e/_c3IZQuuv) |
| SparkFun AS3935 lightning sensor | [Buy on AliExpress](https://s.click.aliexpress.com/e/_c3koAdaZ) |
| DHT22 | [Buy on AliExpress](https://s.click.aliexpress.com/e/_c3J5mlXT) |

## Notes

1. Tuning the lightning sensor's gain took four separate thunderstorms across three weeks, because the only way to test it is to wait for an actual storm and see what the chip logs. Three storms in a row came back with zero detected strikes despite audible thunder, until the cause turned out to be a single register: the chip powers on in `INDOOR` gain, and at this location that setting saturates the front end. Switching to `OUTDOOR` mid-storm took it from zero to 66 lightning events in 14 minutes. `as3935_monitor_loose` is still sitting in `platformio.ini`, waiting for whichever storm shows up next.

![Lightning ring, wide 10/20/30/40 km scale, several strikes, 25/h](images/strikes-2.JPG) ![Lightning ring with an overhead strike and the alert icon lit, wide scale](images/strikes-3.JPG)

2. The pine wood enclosure in `docs/Enclosure Build.md` came out rustic. I'm not much of a carpenter.

## Contact

Feel free to reach out to me on social media: @pakequis in any of them...

You can also send me an email at pakequis (Gmail).
