# Screen Measurement Mode

> A build-time mode that turns the e-paper into a 1:1 physical template:
> centimetre rulers on every edge plus a calibrated 10 cm bar, latched on
> the panel so the board can be powered off and the frozen image used to
> mark cuts (an enclosure, a mounting plate, a bezel).

## What it does

When `SHOW_SCREEN_RULER` is non-zero, `main.cpp` calls
`enterScreenRulerMode()` as the very first thing in `setup()` — before the
IRQ-wake check, before any sensor or WiFi work. That function:

1. Initializes the display.
2. Draws the ruler screen (`updateScreen()`, paged mode, one full refresh).
3. Powers the panel down (`sleepDisplay()` → `hibernate()`).
4. Enters deep sleep with **no wake source armed** (no timer, no AS3935
   `ext1`), so the board stays asleep until a manual reset or power cycle.

The e-paper holds the last image with no power, so the intended workflow is:
flash → let it draw → unplug → use the screen as a stencil.

## What is on the screen

- **Edge rulers** on all four sides, ticks pointing inward: 1 mm (short),
  5 mm (medium), 1 cm (long, numbered). The top and bottom rulers share an
  x origin at the left edge; the left and right rulers share a y origin at
  the top edge.
- **Title**: `Pakéquis - Screen Measurement Test` (the accented `é` is
  drawn by hand — the bundled Adafruit GFX fonts carry no Latin-1 glyphs).
- **Info lines**: calibrated active-area size and `px/cm`.
- **10 cm calibration bar**, centred, with mm ticks. Lay a real ruler on
  it: it must read 100.0 mm. If it does not, the panel needs re-calibrating
  (below).

## Panel geometry and calibration

Panel: Waveshare 7.5" `GxEPD2_750`, 640 × 384 px.

| | Nominal (datasheet) | Calibrated (this unit) |
|---|---|---|
| Active area | 163.2 × 97.92 mm | ≈ 163.7 × 98.2 mm |
| Dot pitch | 0.255 mm/px (isotropic) | ≈ 0.2558 mm/px |
| Scale | 39.216 px/cm | ≈ 39.11 px/cm |

The datasheet pitch is a clean `163.2 / 640 = 0.255` mm, but the first
build's 10 cm bar measured **100.28 mm** with a caliper (tick-centre to
tick-centre). `src/screen_ruler.cpp` therefore applies a single trim
constant to every dimension it draws:

```cpp
constexpr float SCALE_CORRECTION = 100.0f / 100.28f;   // ≈ 0.9972
constexpr float PX_PER_MM = PX_PER_MM_NOMINAL * SCALE_CORRECTION;
```

After the trim the bar was re-measured at 100.0 mm. The scale is
isotropic, so the one horizontal bar validates vertical scaling too.

### Re-calibrating after a panel swap

1. Set `SCALE_CORRECTION` back to `1.0f`, flash the `screen_ruler` env.
2. Measure the 10 cm bar with a caliper, tick-centre to tick-centre.
3. Set `SCALE_CORRECTION = 100.0f / <measured mm>`, reflash, re-measure to
   confirm.

## Usage

```sh
# Draw the ruler (production binary + -DSHOW_SCREEN_RULER=1 + serial on)
pio run -e screen_ruler -t upload

# Return to the normal firmware
pio run -e esp32-s3-devkitc-1 -t upload
```

The `screen_ruler` env `extends` `esp32-s3-devkitc-1`, so it is the exact
production build with the flag (and `APP_DEBUG_SERIAL=1`) added — nothing
else differs.

`SHOW_SCREEN_RULER` defaults to `0` in `config.h` and is wrapped in
`#ifndef`, so a `-D SHOW_SCREEN_RULER=1` build flag overrides it without
editing the file (same pattern as `APP_DEBUG_SERIAL`).

## Files

| File | Purpose |
|---|---|
| `include/screen_ruler.h` | `enterScreenRulerMode()` declaration |
| `src/screen_ruler.cpp` | Drawing, calibration constant, deep-sleep exit |
| `include/config.h` | `SHOW_SCREEN_RULER` flag |
| `src/main.cpp` | `#if SHOW_SCREEN_RULER` branch at the top of `setup()` |
| `platformio.ini` | `[env:screen_ruler]` |

The drawing code is always compiled into the firmware; the flag only
decides whether `main.cpp` reaches it. Its flash cost is negligible
(production uses ~15% of 6.5 MB).
