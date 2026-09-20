# Lithium Battery Power Study

> **Note**: first session of the study. Goal: make it feasible to power
> the circuit from a 2500mAh Li-ion battery, with battery voltage read
> via the ESP32-S3's ADC, and estimate how many days the battery would
> last. Discussion still ongoing. This document records what has been
> measured/decided so far, to continue in another session.

## Goal

1. Define a battery voltage reading circuit (resistive divider on a
   free ADC pin).
2. Estimate how long a 2500mAh battery would power the project, given
   the wake pattern every `DEEP_SLEEP_INTERVAL_SEC` (60s, see
   `config.h`) with WiFi/NTP sync only when `NTP_RESYNC_INTERVAL_MS`
   is due (1h).

## Board context

- Real board: **ESP32-S3-DevKitC-1**, no onboard charging/BMS circuit
  (not a Feather/LiPo-style board).
- The onboard USB-serial bridge is a **WCH** chip ("USB Single Serial",
  VID:PID `1A86:55D3`), shows up as `/dev/ttyACM0` on Linux. Not a
  classic CH340, but the same category (a separate chip from the
  ESP32-S3, powered by the same 3.3V/5V USB rail).
- Free ADC1 pins on this board (avoiding the ones already used: display
  4/10/11/12/16/17, sensors/button 1/2/6/8/9, see `Pin Mapping.md`):
  candidates include **GPIO5** (recommended) and GPIO7. ADC2 (GPIO11-20)
  was ruled out because it shares circuitry with the WiFi radio, which
  this project uses actively before sleeping.

## First session measurements

### Current (user's meter, 10mA resolution, inline USB-C between PC and the DevKit)

| State | Current |
|---|---|
| WiFi on | ~0.2 A |
| Normal update (no WiFi, every 60s) | ~0.03 A |
| Deep sleep | <0.01 A (meter's resolution floor, real value unknown) |

Important: this measurement is *inline on the USB cable* (5V/VBUS side,
before the onboard LDO), so the sleep value already includes the LDO +
USB-serial chip + ESP32-S3 combined. This rules out the pessimistic
scenario of 10-15mA of leakage from the board alone (which had been my
hypothesis before measuring), but doesn't pin down the exact value.
Still need resolution below 10mA.

### Time per phase (`millis()` instrumentation temporarily added to `src/main.cpp` for this measurement, then reverted)

Added three `[TIMING]` prints (gated by the existing `DEBUG_PRINTLN`
macros in `debug.h`, zero cost when `APP_DEBUG_SERIAL=0`, the production
default). Tested by flashing with
`PLATFORMIO_BUILD_FLAGS=-DAPP_DEBUG_SERIAL=1 pio run -t upload` and
reading `/dev/ttyACM0` via pyserial (the normal `pio device monitor`
needs a real interactive TTY, not available in this agent environment).

| Phase | Measured duration |
|---|---|
| Normal wake, no WiFi | **5,600 ms** |
| WiFi phase (connect + NTP + weather/news/crypto fetch) | **10,026 ms** |
| Full wake with WiFi (total) | 15,626 ms (= 10,026 + 5,600, exact match) |
| Draw phase (full e-paper refresh, within the 5,600ms above) | ~5,480 ms |

### Active energy per hour

With 59 normal wakes + 1 WiFi wake per hour, using the currents the
user measured (30mA / 200mA):

- Normal: 59 × (30mA × 5.6s / 3600) ≈ 2.76 mAh
- WiFi: 1 × [(200mA × 10.03s/3600) + (30mA × 5.6s/3600)] ≈ 0.60 mAh
- **Total active: ~3.36 mAh/hour**

Much cheaper than the initial estimate (~10.7mAh/h, based on guessed
durations before measuring).

### Battery duration estimate (2500mAh), by sleep hypothesis

| Sleep (hypothesis) | Total/hour | Duration |
|---|---|---|
| 10mA (worst case, meter's limit) | ~13.3 mAh/h | ~7.8 days |
| 1mA (plausible for the devkit) | ~4.36 mAh/h | ~24 days |
| 0.5mA (good case) | ~3.88 mAh/h | ~27 days |

The bottleneck for the final calculation is just the real sleep value.
The active part is already settled with measured numbers.

## Extra finding: AS3935 noise wakes

On the bench, the lightning sensor (AS3935) fired several wakes from
"disturber" IRQs (noise, not a confirmed strike), ~5-6 in ~75s, far more
frequent than the 60s timer wake. Each one is short (doesn't turn on the
display/WiFi), but if this rate repeats at the final install location
(the bench has significant electromagnetic noise from a nearby PC or
monitor, which may not represent the real environment), the combined
energy of these extra wakes isn't accounted for in the table above. Not
instrumented yet.

## Previously discarded (or not): CH340/LDO

Before measuring, we considered modifying the board to switch off the
regulator/USB-serial chip during sleep (via a reversible switch), to cut
a hypothetical leak of several mA. Since the USB measurement already
showed the whole system (LDO+USB-serial+ESP32-S3) stays below 10mA in
sleep, this invasive modification is probably not necessary. Shelved
unless the real sleep value (not yet measured) turns out high.

## Next steps

1. **Measure sleep below 10mA**: check whether the user's multimeter has
   a separate µA range. If not, the capacitor method (charge a large
   capacitor, disconnect, time the voltage drop with a voltmeter) or an
   INA219/INA226 module.
2. Instrument and measure the real rate/duration of AS3935 noise wakes
   at the final install location (not just on the bench).
3. Close the battery-life estimate with the real sleep value.
4. Design the voltage-reading circuit: resistive divider on GPIO5
   (ADC1), decide whether a high-side MOSFET is worth it to cut the
   divider's leakage during sleep, and decide on the charger approach.

## Code state

The `[TIMING]` instrumentation (3 lines of `millis()`) used to measure
the phases above has already been reverted from the working tree.
`src/main.cpp` no longer has those lines. Reintroducing it (zero cost in
production, gated by the same `DEBUG_PRINTLN` macros) is a quick step if
the measurement needs to be reproduced.
