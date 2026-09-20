## Important warning

**This project is experimental.** Sensor data collected (temperature, humidity, etc.) **are not highly accurate** and should be used only as reference, **not as critical data**.

# Monoboard

![Monoboard dashboard, assembled and running](images/monoboard-1.JPG)

I've had this Waveshare 7.5" e-paper display sitting since 2019, waiting for a project to use it for. This is that project, better late than never.

Dashboard firmware for the panel, driven by an ESP32-S3 DevKitC-1. Weather, local temperature/humidity, lightning strikes, a monthly calendar, a clock, crypto quotes and a news carousel. UI strings switch between PT-BR/EN at compile time. The board wakes from deep sleep on a timer or an AS3935 IRQ, redraws, and goes back to sleep.

## Demo video:
[![Demo Video](https://img.youtube.com/vi/RcL0SpF0Wd4/0.jpg)](https://www.youtube.com/watch?v=RcL0SpF0Wd4)

## Hardware

- [ESP32-S3 DevKitC-1](https://s.click.aliexpress.com/e/_c3HgUNVP)
- [Waveshare 7.5" e-paper display](https://s.click.aliexpress.com/e/_c3IZQuuv)
- [AS3935 module](https://s.click.aliexpress.com/e/_c3koAdaZ)
- [DHT22 module](https://s.click.aliexpress.com/e/_c3J5mlXT)

Affiliate links (AliExpress): buying through them costs you nothing extra and gives me a small commission. Board specs confirmed via `esptool flash_id` are in `docs/Pin Mapping.md`.

## Functionalities

- **E-paper display**: Waveshare 7.5" (640 × 384 px, landscape). Refreshes periodically to keep the panel from ghosting over time. See `docs/API Reference.md`.
- **Weather & local sensors**: 6-hour forecast plus local temperature and humidity.
- **Lightning detection**: shows strike distance, an hourly strike count and recent history, tuned to ignore local electrical noise. See `docs/Lightning Detection.md`.
- **Monthly calendar**, **clock**, **crypto quotes** (BTC/ETH) and a **news carousel**.
- **Screen measurement mode**: an optional build that turns the panel into a printable ruler, used to fit the wood enclosure. See `docs/Screen Measurement Mode.md`.
- **Power management**: sleeps most of the time and only wakes for a scheduled update, a lightning strike, or the hourly clock sync.
- **Language**: switches between Portuguese and English at compile time, no code changes needed.

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
- **Framework**: Arduino, targeting the ESP32-S3.
- **Libraries**: GxEPD2 for the e-paper display, plus the DHT and SparkFun AS3935 libraries for the sensors and ArduinoJson for the weather/news/crypto API calls.
- **Extra builds**: a few diagnostic and calibration builds exist alongside the normal firmware (screen ruler, lightning sensor monitor), see `platformio.ini`.

## Notes

1. Tuning the lightning sensor's gain took four separate thunderstorms across three weeks, because the only way to test it is to wait for an actual storm and see what the chip logs. Three storms in a row came back with zero detected strikes despite audible thunder, until the cause turned out to be a single register: the chip powers on in `INDOOR` gain, and at this location that setting saturates the front end. Switching to `OUTDOOR` mid-storm took it from zero to 66 lightning events in 14 minutes. `as3935_monitor_loose` is still sitting in `platformio.ini`, waiting for whichever storm shows up next.

![Lightning ring, wide 10/20/30/40 km scale, several strikes, 25/h](images/strikes-2.JPG) ![Lightning ring with an overhead strike and the alert icon lit, wide scale](images/strikes-3.JPG)

2. The pine wood enclosure in `docs/Enclosure Build.md` came out rustic. I'm not much of a carpenter.

## Contact

Feel free to reach out to me on social media: @pakequis in any of them...

You can also send me an email at pakequis (Gmail).
