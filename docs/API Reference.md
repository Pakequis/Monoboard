# API Reference — Display Manager

> Complete usage guide for the `display_manager` module.

---

## Overview

The `display_manager` module abstracts all operations with the Waveshare 7.5" e-paper display. It manages initialization, drawing (text, lines, shapes), screen updates, and entry into low-power mode.

### Module files

| File | Purpose |
|---------|--------|
| `include/display_manager.h` | Function declarations and global `display` variable |
| `src/display_manager.cpp` | Implementation with ESP32-S3 pin mapping |

### Global display instance

```cpp
extern GxEPD2_BW<GxEPD2_750, GxEPD2_750::HEIGHT> display;
```

The `display` variable is global and can be accessed from any file that includes `display_manager.h`. It exposes all Adafruit GFX library methods (such as `display.drawPixel()`, `display.drawRoundRect()`, etc.) not explicitly listed below.

---

## Typical usage flow (from `main.cpp`)

```cpp
#include "config.h"
#include "debug.h"
#include "display_manager.h"
#include "time_manager.h"
#include "wifi_manager.h"
#include "dashboard_manager.h"
#include "content_manager.h"
#include "local_sensors.h"
#include "as3935_lightning.h"

void setup()
{
  DEBUG_BEGIN(SERIAL_BAUD_RATE);                    // no-op when APP_DEBUG_SERIAL=0 (see debug.h)

  if (isWakeFromAs3935Irq())                        // AS3935 IRQ wake: short-circuit path
  {
    handleLightningIrqWake();                       //    Record the strike (if it's a real one), no display/WiFi
    if (isRedrawDue())
    {
      resetCloseStrikeRedrawLatch();               //    Normal-cadence redraw -- re-arm the latch below
      // fall through to a normal full redraw
    }
    else if (shouldForceEarlyRedraw())               //    Close strike within LIGHTNING_ALERT_KM (energy irrelevant):
    {                                                 //    force one early redraw, latched so a burst of
      // fall through to a forced-early redraw          //    several doesn't force one each
    }
    else
    {
      armAs3935IrqWakeup();
      esp_sleep_enable_timer_wakeup(DEEP_SLEEP_INTERVAL_SEC * 1000000ULL);
      esp_deep_sleep_start();
      return;
    }
  }
  else
  {
    resetCloseStrikeRedrawLatch();                 // Normal-cadence wake (timer) also re-arms the latch
  }

  initDisplay();                                    // 1. Initialize display
  initLocalSensors();

  applyTimezone();                                  // 2. Re-apply TZ/SNTP config (cheap, no WiFi needed)

  bool syncDue = isTimeSyncDue();
  if (syncDue)                                       //    Only bring up WiFi when a sync is actually due
  {
    if (wifiConnect())
    {
      syncTimeViaNTP();                              //    Blocking NTP sync (see time_manager)
      fetchNetworkContent();                         //    Per-content-type fetch (see content_manager)
      wifiDisconnect();
    }
  }

  readLocalSensors();                               // Always, no network needed (see local_sensors)
  advanceNewsCarousel();                            //    Rotates every redraw, not just every sync (see content_manager)

  drawDashboard();                                  // 3. Draw + update (see dashboard_manager)
  clearLightningAlert();                            //    One-shot alert icon consumed by this redraw
  sleepDisplay();                                   // 4. Power off display

  armAs3935IrqWakeup();                             //    ext1 wake source, in addition to the timer
  esp_sleep_enable_timer_wakeup(DEEP_SLEEP_INTERVAL_SEC * 1000000ULL);
  esp_deep_sleep_start();                            // 5. Deep sleep
}
```

This mirrors the actual control flow in `src/main.cpp` (debug logging and the redraw-due timestamp bookkeeping omitted here for brevity — see that file for the exact wording and error-path messages). `drawDashboard()` (in `dashboard_manager.cpp`) is what actually calls `updateScreen()` with the full header/content-grid drawing callback -- there is no footer. `writeTextCentered()`/`drawRect()` below are still available as general-purpose primitives for content types not yet built.

### Other modules

| Module | Purpose |
|---------|--------|
| `time_manager.h/.cpp` | `applyTimezone()` (cheap, every wake, no WiFi needed) + `isTimeSyncDue()` (checks `NTP_RESYNC_INTERVAL_MS`, currently 1h, no WiFi needed) + `syncTimeViaNTP()` (blocking, requires WiFi already connected). Persists last-sync epoch across deep sleep via `RTC_DATA_ATTR`. Exposes `getTimeString()` (`"HH:MM"`), `getDateTimeString()` (`"Weekday, Month DD, YYYY HH:MM"`), `getCurrentTime()`, and `getCalendarMonthInfo()` — `getDateTimeString()`/`getTimeString()` return a placeholder (`"--:--"`) instead of formatting a bogus epoch-0 value when `time(nullptr)` is still below `NTP_EPOCH_VALID_THRESHOLD` (i.e. before the first successful NTP sync); `getCalendarMonthInfo()` returns 0/0 in the same case, signalling the caller to skip the calendar grid. All are fixed-buffer output (no `String`). |
| `wifi_manager.h/.cpp` | `wifiConnect()` / `wifiDisconnect()`, plus `isWifiConnected()` for the header's status line — persists the last connection result across deep sleep (`RTC_DATA_ATTR`) so it stays accurate on wake cycles that skip WiFi entirely. |
| `dashboard_manager.h/.cpp` | Draws the full dashboard: a one-line header (title, WiFi status, firmware version), then the 2x2 content grid -- top row (weather forecast + sensor temp/humidity, lightning box, calendar), bottom row (news headlines, clock + a reserved empty box) -- using `display_manager` primitives. No footer. |
| `content_manager.h/.cpp` | RTC-memory caches for internet content: the 6-card weather forecast (code + temperature + `HHh` hour label per interval) and a 9-headline news carousel (3 buffers of 3, one buffer shown per screen redraw). `fetchNetworkContent()`, `getWeatherForecastIconChar()`/`getWeatherForecastHourLabel()`/`getWeatherForecastTemperatureNumberLabel()`, `advanceNewsCarousel()`, `getNewsHeadline()`. |
| `lib/news_client` | `fetchTopHeadlines()` -- fetches the Google News RSS feed (`NEWS_API_URL`, locale-aware via `strings.h`) and streams it through a small tag-matching state machine (no full-body buffering, the feed runs well over 100KB) to pull out just the `<title>` of each `<item>`, cleaned up (source suffix stripped, HTML entities decoded, accents/smart punctuation transliterated to ASCII) for the display font. |
| `local_sensors.h/.cpp` | `initLocalSensors()` once at boot, `readLocalSensors()` every wake (no network) + `getTempHumidityText()` for the DHT22 reading. AS3935 lightning: `handleLightningIrqWake()`/`onConfirmedLightningStrike()` record confirmed strikes (km + energy) into a ring buffer (`getStrikeHistoryCount()`/`getStrikeHistoryEntry()`, up to `STRIKE_HISTORY_COUNT`, cleared after `STRIKE_RESET_TIMEOUT_SEC` of silence) and a separate trailing-window buffer for `getLightningRateText()` (strikes in the last `LIGHTNING_RATE_WINDOW_SEC`, shown in the header). A strike at or within `LIGHTNING_ALERT_KM` (distance only, energy irrelevant) also sets the alert-icon flag (`isLightningAlertActive()`/`clearLightningAlert()`) and, on an IRQ wake where a redraw wasn't otherwise due, forces one early redraw -- `shouldForceEarlyRedraw()`/`resetCloseStrikeRedrawLatch()` latch this to at most one forced redraw per burst, re-armed on the next normal-cadence redraw. Each field independently shows a placeholder if its sensor isn't connected or the clock hasn't synced. |
| `debug.h` | `DEBUG_BEGIN`/`DEBUG_PRINT`/`DEBUG_PRINTLN` macros, gated by `APP_DEBUG_SERIAL` in `config.h` — expand to real `Serial.*` calls or nothing at all, compiled out entirely when off. |
| `strings.h` | `STR_*` display-string keys, switched between PT-BR/EN at compile time via `APP_LANGUAGE`. Both language blocks must define the same set of keys. |
| `lib/weather_client` | Fetches weather code + temperature + `is_day` (Open-Meteo) for the next `WEATHER_FORECAST_HOURS` complete hours shown in the forecast box, plus each interval's local `HHh` hour label. The interval currently in progress is excluded, so `WEATHER_API_URL` requests one extra interval (`forecast_hours=7`) to back the 6 displayed cards. `is_day` picks the day/night icon variant. |
| `lib/weather_icon_map` | Maps an Open-Meteo WMO weather code + day/night flag to a Weather Icons glyph character. Pure logic, no Arduino dependency — covered by host-native unit tests (`pio test -e native`). |
| `lib/calendar_math` | Calendar grid-placement math: which (row, col) a given day number occupies in the calendar box's grid, and month/weekday facts derived from a year+month. Pure logic, no Arduino dependency — covered by host-native unit tests. |
| `lib/text_cleanup` | News-headline text cleanup: strips the RSS feed's trailing source-name suffix, decodes HTML entities, and transliterates accented/smart-punctuation UTF-8 to plain ASCII for the display font. Pure logic, no Arduino dependency — covered by host-native unit tests. |
| `lib/text_layout` | Word-wrap and truncate-with-ellipsis logic for fitting text into a fixed pixel width, given an injected width-measuring callback instead of a direct display dependency. Pure logic, no Arduino dependency — covered by host-native unit tests. |
| `lib/dht22_sensor`, `lib/as3935_lightning` | Sensor drivers backing `local_sensors`. |
| `lib/dseg_fonts`, `lib/weather_icons_font` | The clock's seven-segment `GFXfont` and the weather icon box's icon `GFXfont`, each in its own folder with its own license — see "Available fonts" below. |

---

## Functions

### `initDisplay()`

Initializes the e-paper display via SPI.

```cpp
void initDisplay();
```

**Functionality**:
- Initializes the GxEPD2 display object (pins are defined in the global `display` constructor at file scope, this function calls `display.init()` to bring up the SPI bus and panel driver, then sets rotation)
- Calls `display.init()` with the library's own diagnostic serial bitrate (tied to `APP_DEBUG_SERIAL` in `config.h` — `SERIAL_BAUD_RATE` in a debug build, `0` when serial is compiled out), not a fixed display communication speed
- Sets rotation to `0` — the panel's native orientation, which is **landscape** (640×384, width > height), not portrait

**Usage**: should be called **once** in `setup()`.

---

### `clearScreen()`

Clears the screen by filling it with white.

```cpp
void clearScreen();
```

**Note**: operates on the internal buffer. The change appears on screen only after calling `updateScreen()`.

---

### `writeText(x, y, text, font, color)`

Writes text at a specific position.

```cpp
void writeText(int16_t x, int16_t y, const char* text,
               const GFXfont* font = nullptr,
               uint16_t color = GxEPD_BLACK);
```

| Parameter | Type    | Description                     | Default       |
|-----------|---------|---------------------------------|--------------|
| `x`       | int16_t | X coordinate (column)           | —            |
| `y`       | int16_t | Y coordinate (baseline)         | —            |
| `text`    | string  | Text to display                 | —            |
| `font`    | GFXfont*| Pointer to font                 | `nullptr` (default) |
| `color`   | uint16_t| Text color                      | `GxEPD_BLACK` |

**Example**:
```cpp
writeText(50, 100, "Hello World", &FreeMonoBold9pt7b, GxEPD_BLACK);
```

---

### `writeTextCentered(text, font, color)`

Writes text centered on the screen (horizontally and vertically).

```cpp
void writeTextCentered(const char* text,
                       const GFXfont* font = nullptr,
                       uint16_t color = GxEPD_BLACK);
```

| Parameter | Type    | Description                     | Default       |
|-----------|---------|---------------------------------|--------------|
| `text`    | string  | Text to display                 | —            |
| `font`    | GFXfont*| Pointer to font                 | `nullptr`       |
| `color`   | uint16_t| Text color                      | `GxEPD_BLACK` |

**Example**:
```cpp
char dateTimeBuf[32];
getDateTimeString(dateTimeBuf, sizeof(dateTimeBuf));
writeTextCentered(dateTimeBuf, &FreeMonoBold9pt7b, GxEPD_BLACK);
```

---

### `drawLine(x1, y1, x2, y2, color)`

Draws a line between two points.

```cpp
void drawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2,
              uint16_t color = GxEPD_BLACK);
```

**Example**:
```cpp
drawLine(10, 10, display.width() - 10, display.height() - 10);
```

---

### `drawRect(x, y, w, h, color)`

Draws a hollow rectangle (outline only).

```cpp
void drawRect(int16_t x, int16_t y, int16_t w, int16_t h,
              uint16_t color = GxEPD_BLACK);
```

**Example**:
```cpp
drawRect(DISPLAY_FRAME_MARGIN, DISPLAY_FRAME_MARGIN,
         display.width() - DISPLAY_FRAME_MARGIN * 2,
         display.height() - DISPLAY_FRAME_MARGIN * 2,
         GxEPD_BLACK);
```

---

### `fillRect(x, y, w, h, color)`

Draws a filled rectangle.

```cpp
void fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
              uint16_t color = GxEPD_BLACK);
```

---

### `drawCircle(x, y, r, color)`

Draws a hollow circle.

```cpp
void drawCircle(int16_t x, int16_t y, int16_t r,
                uint16_t color = GxEPD_BLACK);
```

| Parameter | Type    | Description      |
|-----------|---------|----------------|
| `x`, `y`  | int16_t | Circle center   |
| `r`       | int16_t | Radius in pixels |

---

### `fillCircle(x, y, r, color)`

Draws a filled circle.

```cpp
void fillCircle(int16_t x, int16_t y, int16_t r,
                uint16_t color = GxEPD_BLACK);
```

---

### `drawDashedCircle(x, y, r, dashLength, gapLength, color)`

Draws a circle outline as alternating drawn/skipped short arcs (Adafruit_GFX has no built-in dashed-line primitive).

```cpp
void drawDashedCircle(int16_t x, int16_t y, int16_t r,
                      int16_t dashLength, int16_t gapLength,
                      uint16_t color = GxEPD_BLACK);
```

| Parameter    | Type    | Description                              |
|--------------|---------|-------------------------------------------|
| `x`, `y`     | int16_t | Circle center                             |
| `r`          | int16_t | Radius in pixels                          |
| `dashLength` | int16_t | Length of each drawn arc, in pixels        |
| `gapLength`  | int16_t | Length of each skipped arc, in pixels      |

---

### `drawArc(x, y, r, gapCenterAngleRad, gapLengthPx, color)`

Draws a circle outline with a single gap, useful for a ring that needs to leave room for a label without overlapping it.

```cpp
void drawArc(int16_t x, int16_t y, int16_t r,
             float gapCenterAngleRad, int16_t gapLengthPx,
             uint16_t color = GxEPD_BLACK);
```

| Parameter            | Type    | Description                                                   |
|----------------------|---------|-----------------------------------------------------------------|
| `x`, `y`             | int16_t | Circle center                                                   |
| `r`                  | int16_t | Radius in pixels                                                |
| `gapCenterAngleRad`  | float   | Angle (radians, standard `atan2` convention) the gap is centered on |
| `gapLengthPx`        | int16_t | Gap width, in pixels of arc length (converted to an angle internally, so it covers the same arc-length at any radius) |

---

### `drawLightningBolt(x, y, w, h, color)`

Draws a stylized lightning bolt (2 overlapping filled triangles) inside the bounding box `(x, y)`-`(x+w, y+h)`. Used as the lightning box's close-strike alert icon (`isLightningAlertActive()`, `local_sensors.h`).

```cpp
void drawLightningBolt(int16_t x, int16_t y, int16_t w, int16_t h,
                       uint16_t color = GxEPD_BLACK);
```

| Parameter | Type    | Description                          |
|-----------|---------|----------------------------------------|
| `x`, `y`  | int16_t | Top-left corner of the bounding box     |
| `w`, `h`  | int16_t | Bounding box width/height, in pixels    |

---

### `updateScreen(drawFunc)`

Updates the display by executing a redraw callback in paged mode.

```cpp
typedef void (*DrawFunction)();
void updateScreen(DrawFunction drawFunc);
```

**Behavior**:
- Executes the callback on each page, redrawing the entire frame
- In the callback, use `clearScreen()` + all desired drawing calls

**Example**:
```cpp
updateScreen([]{ 
  clearScreen();
  drawRect(DISPLAY_FRAME_MARGIN, DISPLAY_FRAME_MARGIN,
           display.width() - DISPLAY_FRAME_MARGIN * 2,
           display.height() - DISPLAY_FRAME_MARGIN * 2,
           GxEPD_BLACK);
  char dateTimeBuf[32];
  getDateTimeString(dateTimeBuf, sizeof(dateTimeBuf));
  writeTextCentered(dateTimeBuf, &FreeMonoBold9pt7b, GxEPD_BLACK);
});
```

**Note**: this process takes a few seconds (e-paper refresh is inherently slow).

---

### `conditionPanel(cycles)`

Clears accumulated ghosting by flushing the panel with `cycles` black↔white
full-refresh pairs, leaving it white.

```cpp
void conditionPanel(uint8_t cycles);
```

**Why**: the GDEW075T8 accumulates a faint ghost of static regions (the
frame, fixed labels) under weeks of near-identical full refreshes. Driving
every pixel through its full range a few times clears the retained charge.

**Cost**: ~9 s per cycle (two ~4.5 s full refreshes). Blocking.

**Usage**: `main.cpp` calls this from `maybeConditionPanel()` right before
`drawDashboard()` — every `DISPLAY_CONDITION_INTERVAL_SEC` worth of redraws
(counter-based, so no valid clock is needed) and, when
`DISPLAY_CONDITION_ON_COLD_BOOT` is set, once per cold boot. The dashboard
is then drawn over the cleared panel, so the visible result is unchanged.
All three knobs are in `config.h`.

---

### `sleepDisplay()`

Powers off the display and puts it into hibernation mode to save energy.

```cpp
void sleepDisplay();
```

**What it does**:
1. `display.powerOff()` → powers off the panel driver
2. `display.hibernate()` → puts the controller in ultra-low-power mode

**Note**: the image remains visible even without power (e-paper characteristic).

---

## Useful constants

| Constant        | Value           | Description                  |
|------------------|-----------------|------------------------------|
| `GxEPD_BLACK`    | `0x0000`        | Black color                  |
| `GxEPD_WHITE`    | `0xFFFF`        | White color                 |
| `display.width()` | 640            | Display width in px          |
| `display.height()`| 384            | Display height in px         |

---

## Available fonts

Include the desired font header and pass its pointer as a parameter:

```cpp
#include <Fonts/FreeMonoBold9pt7b.h>
#include "dseg7_classic_bold.h"
#include "weathericons_32.h"
```

| Font                           | Header                          | Style |
|----------------------------------|----------------------------------|-----------|
| FreeMonoBold 9pt                 | `<Fonts/FreeMonoBold9pt7b.h>`    | Monospaced, bold, 9pt — labels, header line, calendar, weather box hour/temperature, news headlines |
| DSEG7 Classic Bold (44px)        | `"dseg7_classic_bold.h"` (`lib/dseg_fonts/`) | Seven-segment digital-clock style — the clock in the bottom-right quadrant, sized to roughly match the reserved box's own width below it. Licensed under SIL OFL-1.1, full text in `lib/dseg_fonts/DSEG-LICENSE.txt`. Regenerated at any pixel size via `tools/convert_dseg_font.py`. |
| Weather Icons (32px)             | `"weathericons_32.h"` (`lib/weather_icons_font/`) | Weather-icon symbol font (erikflowers/weather-icons) — chars `'1'`-`'7'`, `'9'` each render a distinct icon glyph, `'8'`/`';'` are the night variants of `'1'`/`'9'` (see `weatherCodeToIconChar()` in `lib/weather_icon_map/weather_icon_map.cpp` for the WMO-code + day/night mapping), `':'` renders blank. Licensed under SIL OFL-1.1, full text in `lib/weather_icons_font/WEATHER-ICONS-LICENSE.txt`. |

Every font here only covers ASCII `0x20`–`0x7E` (no accented characters) — PT-BR strings in `strings.h` are written without accents to stay within this range. The display itself (`GxEPD2_BW`) is strictly 1-bit monochrome: DSEG7's "ghost segment" look (visible on the font's own specimen page) depends on grayscale and doesn't survive conversion to this bitmap format — the DSEG font used here only ever draws fully "on" segments.

**Example**:
```cpp
writeText(0, 50, "Text in 9pt", &FreeMonoBold9pt7b);
writeText(0, 100, "12:34", &DSEG7_Classic_Bold_44);
```

---

## Build and upload

### Build for ESP32-S3
```bash
pio run -e esp32-s3-devkitc-1
```

### Upload
```bash
pio run -e esp32-s3-devkitc-1 -t upload
```

### Host-native unit tests

Pure-logic modules (`lib/weather_icon_map`, `lib/calendar_math`, `lib/text_cleanup`, `lib/text_layout`) have Unity unit tests that build and run on this machine's own compiler, no ESP32 board involved:
```bash
pio test -e native
```

---

## Deep sleep

After `sleepDisplay()`, call `esp_deep_sleep_start()` to put the ESP32 into the lowest possible power mode (~5–10 µA). The chip wakes on two sources: a timer (`esp_sleep_enable_timer_wakeup(DEEP_SLEEP_INTERVAL_SEC * 1000000ULL)`), and an `ext1` GPIO wake on the AS3935's IRQ pin (`armAs3935IrqWakeup()`, from `as3935_lightning.h`) — both must be (re-)armed every cycle, since deep sleep resets peripheral configuration. `main.cpp` calls both right before `esp_deep_sleep_start()`. Every wake source runs `setup()` again from scratch (a full reboot; only `RTC_DATA_ATTR` variables and the RTC-backed clock survive). `isWakeFromAs3935Irq()` tells `main.cpp` whether this wake was a lightning-sensor interrupt, which takes a short-circuit path (record the strike, skip the display/WiFi entirely unless a redraw is separately overdue). The reset button still works too, but it isn't a designed wake path.

```cpp
esp_sleep_enable_timer_wakeup(DEEP_SLEEP_INTERVAL_SEC * 1000000ULL);
sleepDisplay();
esp_deep_sleep_start();  // Never returns
```

---

## Important notes

1. **Coordinates**: origin `(0, 0)` is the top-left corner
2. **Persistence**: e-paper image remains visible even with the display powered off
3. **Refresh time**: full refresh takes 2–8 seconds (depending on the model)
4. **Avoid frequent refresh**: each update wears the display (~1,000,000 cycle lifetime)
