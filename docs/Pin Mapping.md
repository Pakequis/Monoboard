# Pin Mapping

> Wiring reference for the ESP32-S3 DevKitC-1 board this project actually
> builds for (`platformio.ini` only defines the `esp32-s3-devkitc-1` env).

## Display: Waveshare 7.5" (GxEPD2_750)

- **Resolution**: 640 x 384 pixels
- **Interface**: SPI
- **Voltage**: 3.3V (do not use 5V)

| Display | GPIO | Function     | Notes                        |
|---------|------|--------------|-------------------------------|
| BUSY    | 4    | Busy status  | 4.7kΩ pull-up recommended     |
| RST     | 16   | Reset        |                                |
| DC      | 17   | Data/Command |                                |
| CS      | 10   | Chip Select  | ESP32-S3 default SS           |
| CLK     | 12   | SPI Clock    | SCK                            |
| DIN     | 11   | SPI MOSI     |                                |
| GND     | GND  | Ground       |                                |
| 3.3V    | 3.3V | Power        |                                |

Hardcoded in `src/display_manager.cpp` (no `#ifdef`, since only one board
target exists):

```cpp
static const uint8_t PIN_BUSY = 4;
static const uint8_t PIN_RST  = 16;
static const uint8_t PIN_DC   = 17;
static const uint8_t PIN_CS   = 10;
static const uint8_t PIN_SCK  = 12;
static const uint8_t PIN_MOSI = 11;
```

### Block diagram

```
┌─────────────────────┐       ┌──────────────────────┐
│                      │       │                      │
│      ESP32-S3        │  SPI  │  Waveshare 7.5"      │
│                      │◄─────►│  E-Paper Display     │
│  GPIO 4  ───────────►│ BUSY  │                      │
│  GPIO 16 ───────────►│ RST   │                      │
│  GPIO 17 ───────────►│ DC    │                      │
│  GPIO 10 ───────────►│ CS    │                      │
│  GPIO 12 ───────────►│ CLK   │                      │
│  GPIO 11 ───────────►│ DIN   │                      │
│  GND     ───────────►│ GND   │                      │
│  3.3V    ───────────►│ 3.3V  │                      │
│                      │       │                      │
└─────────────────────┘       └──────────────────────┘
```

## Local sensors & manual refresh button

| Module          | GPIO | Function                     | Notes |
|-----------------|------|-------------------------------|-------|
| Refresh button  | 1    | `ext0` deep-sleep wake input  | Button to GND when pressed; internal pull-up enabled in software (`refresh_button.cpp`). Wakes on LOW. |
| DHT22           | 2    | Data (1-wire)                 | Needs a ~10kΩ pull-up to 3.3V — many breakout modules already include this on-board; check before wiring a bare sensor. |
| AS3935 breakout | 6    | IRQ, `ext1` deep-sleep wake   | The board wakes directly on this pin the instant the chip signals an event (`armAs3935IrqWakeup()`/`isWakeFromAs3935Irq()` in `as3935_lightning.cpp`), instead of only catching strikes on the next timer-driven poll. |
| AS3935 breakout | 8    | I2C SDA (module's `MOSI` pin) | Shared I2C bus, default ESP32-S3 Arduino core pin. |
| AS3935 breakout | 9    | I2C SCL                       | Shared I2C bus, default ESP32-S3 Arduino core pin. |

All five pins avoid the display's GPIO 4/10/11/12/16/17, the native-USB
GPIO 19/20, and the strapping pins GPIO 0/3/45/46.

### AS3935 breakout: full pinout (fixed logic-level straps)

The specific breakout in hand (silkscreen: `A1`, `A0`, `EN_V`, `IRQ`, `SI`,
`CS`, `MISO`, `MOSI`, `SCL`, `GND`, `VCC`) ties several extra pins to fixed
levels to put the chip in I2C mode (no on-board pull-ups on SDA/SCL —
external ~4.7kΩ pull-ups to VCC are required):

| Pin  | Tied to        | Purpose |
|------|----------------|---------|
| VCC  | 3.3V           | Power (not 5V) |
| GND  | GND            | Ground |
| EN_V | GND            | Regulator/interface enable |
| SI   | VCC            | Interface select: HIGH = I2C mode, LOW = SPI mode |
| CS   | GND            | SPI chip-select, unused in I2C mode |
| MISO | Not connected  | SPI-only pin, unused in I2C mode — currently left floating |
| A0   | VCC            | I2C address bit 0 |
| A1   | VCC            | I2C address bit 1 — both A0/A1 HIGH gives I2C address `0x03` (matched explicitly in `as3935_lightning.cpp`: `SparkFun_AS3935 lightning(defAddr)`) |

Antenna tuning capacitance (`AS3935_TUNE_CAP` in `config.h`) is `0` pF for
this specific physical unit — re-measure if the sensor is ever swapped.
