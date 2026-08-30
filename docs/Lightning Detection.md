# Lightning Detection

> How the SparkFun AS3935 is driven: the deep-sleep IRQ wake, the
> strikes/hour header metric, the sensitivity tuning that keeps indoor
> electrical noise from registering as strikes, and the `as3935_monitor`
> diagnostic build.

Wiring and the breakout's fixed straps are in `Pin Mapping.md`. Antenna
tuning (`AS3935_TUNE_CAP`) is `0` pF for this physical unit.

## Wake and read flow

The AS3935 IRQ pin is an `ext1` deep-sleep wake source (GPIO 6). On an
IRQ wake `main.cpp` runs the short-circuit path in `handleLightningIrqWake()`:
bus bring-up only (**not** `initAs3935()` — a full init writes REG0x03,
whose read-modify-write would consume the unread strike interrupt), read
the interrupt register, and if it is a confirmed `LIGHTNING` event record
it via `onConfirmedLightningStrike()`. No display, no WiFi. The board then
goes straight back to sleep unless a redraw is due or the strike was close
(`LIGHTNING_ALERT_KM`).

Full configuration (`initAs3935()`) runs only on a cold boot or a timer
wake, when no unread strike interrupt is pending.

## Strikes/hour header metric

The "Raios" box header shows a **sliding-window count**: confirmed strikes
in the trailing `LIGHTNING_RATE_WINDOW_SEC` (3600 s). Timestamps go into a
separate RTC-memory ring buffer, `strikeRateTimestamps`, capped at
`LIGHTNING_RATE_MAX_SAMPLES` (120). `getLightningRateText()` counts the
entries still inside the window.

This is deliberately decoupled from the 5-entry strike **history** used
for the distance-ring overlay (`STRIKE_HISTORY_COUNT`), which caps low for
visual reasons unrelated to how many strikes actually landed in an hour.

### The "stuck at the cap" failure and its fix

The window count has no absolute reset — it is meant to self-expire via
`(now - entry) <= LIGHTNING_RATE_WINDOW_SEC`. `time_t` is signed, so any
stored timestamp **at or ahead of** the current clock produces a negative
difference that always passes the window test and never leaves it. A
strike recorded on an IRQ wake while the RTC had drifted forward, then an
NTP correction pulling the clock back, was enough to pin the header at the
buffer cap indefinitely (seen at 40, then 120 after the cap was raised)
under a clear sky. Two guards address it:

- `getLightningRateText()` ignores any entry with `entry > now`.
- `resetStrikeStateIfStale()` clears the rate buffer (not just the
  history) on the `STRIKE_RESET_TIMEOUT_SEC` no-strike timeout, as a
  backstop.
- `syncTimeViaNTP()` rejects an NTP result below `NTP_EPOCH_PLAUSIBLE_MIN`
  (2025-01-01) instead of only checking it is past the epoch, so a bad
  `pool.ntp.org` response can no longer poison the clock and, through it,
  the stored strike timestamps.

## Sensitivity tuning

The AS3935 powers on with every noise-rejection knob at its most
permissive setting and the SparkFun library never changes them. Left that
way, a single indoor electrical transient (a brushed motor, a switching
supply, a nearby car ignition) raises a `LIGHTNING` interrupt. An hour of
`as3935_monitor` logging under a clear sky recorded **31 false "strikes"**
in two bursts, distance marching 10 km → 1 km with energy up to ~500000 —
the signature of local interference, not weather.

`initAs3935()` now overrides the power-on defaults (all in `config.h`):

| Constant | Power-on | Set to | Meaning |
|---|---|---|---|
| `AS3935_MIN_STRIKES` | 1 | **5** | Confirmed strikes the chip must see in its ~17-minute internal window before it fires the interrupt. Only 1/5/9/16 are valid. 5 discards a lone transient; a real storm produces strikes continuously so the delay is small. |
| `AS3935_WATCHDOG_THRESHOLD` | 2 | **3** | 0–10; higher rejects more non-lightning waveforms, at the cost of missing weak/distant real strikes. |
| `AS3935_SPIKE_REJECTION` | 2 | **3** | 0–11; same trade-off, for spike-shaped waveforms. |
| `AS3935_NOISE_LEVEL` | 2 | 2 | 1–7 noise-floor reference. Unchanged — monitoring logged zero `NOISE_TOO_HIGH` events. |

Raise the thresholds further if false strikes persist; lower them toward
the defaults if real nearby storms get missed.

### Firmware-side overhead guard

The chip reports distance as a **running minimum** over its event window,
so one strong local transient pins it at 1 km and later events hold it
there. `onConfirmedLightningStrike()` discards a strike at
`<= AS3935_OVERHEAD_SANITY_KM` (1 km) whose energy exceeds
`AS3935_OVERHEAD_MAX_PLAUSIBLE_ENERGY` (200000) — real overhead strikes on
this sensor read energy ~17000. Set the energy ceiling to `0` to disable.

## Strike history

`onConfirmedLightningStrike()` also stores `(km, energy)` in a 5-entry RTC
ring buffer for the distance-ring overlay. `resetStrikeStateIfStale()`
clears it (and the rate buffer) after `STRIKE_RESET_TIMEOUT_SEC` (2 h) of
no confirmed strike. A verification build can shorten that with
`-D STRIKE_RESET_TIMEOUT_SEC=8UL`.

## `as3935_monitor` diagnostic build

`src/test_as3935_monitor.cpp` / `[env:as3935_monitor]` — a standalone
sketch (own `setup()`/`loop()`, never linked into the production
firmware). It keeps the board awake and prints one serial line per
AS3935 interrupt with the chip's own classification
(`NOISE_TOO_HIGH` / `DISTURBER` / `LIGHTNING`) plus distance and energy,
and dumps the config registers at boot via `dumpAs3935Config()`. Use it to
tell real strikes from interference: a genuine strike near 1 km comes with
thunder within seconds; a run of `LIGHTNING` lines under a clear sky, all
at a fixed distance with clustered energy, is the sensor misclassifying a
local source.

```sh
pio run -e as3935_monitor -t upload
pio device monitor -e as3935_monitor
```

## Files

| File | Purpose |
|---|---|
| `lib/as3935_lightning/` | AS3935 bus/IRQ/read wrappers, `dumpAs3935Config()` |
| `src/local_sensors.cpp` | Rate buffer, strike history, stale reset, overhead guard |
| `src/time_manager.cpp` | NTP sync with the `NTP_EPOCH_PLAUSIBLE_MIN` floor |
| `include/config.h` | All `LIGHTNING_*` / `AS3935_*` / `STRIKE_*` constants |
| `src/test_as3935_monitor.cpp` | `as3935_monitor` diagnostic sketch |
