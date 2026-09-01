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

### Flash logging

The monitor also appends every heartbeat and every non-`NOISE_TOO_HIGH`
event to `/as3935.log` on the LittleFS partition, so the board can be left
on a bare USB charger with no host attached (a host raises the sensor's
noise rate — see below) and the record read back later. On boot the file
is streamed to serial wrapped in `----- FLASHLOG DUMP START/END -----`
markers, then new lines are appended; it is never truncated and
accumulates across power cycles. A boot counter in NVS labels each
power-on segment, since `millis()` resets with it. **Opening the serial
port resets this board** (native USB CDC), which ends the live run — but
the flash log survives, so reconnecting to read it costs only the current
segment.

## Diagnostic runs

### 2026-08-31 — real thunderstorms, count lower than expected

`as3935_monitor` left running ~14 h 49 min on a bare USB charger through
an evening of confirmed thunderstorms (thunder audible, rain). Compiled
sensitivity was the current `config.h`: `MIN_STRIKES=5`, `watchdog=3`,
`spikeRejection=3`, `noiseLevel=2`, indoor mode, disturber mask on,
`tuneCap=0`.

Persisted totals for the run:

| Class | Count | Notes |
|---|---|---|
| `DISTURBER` | 0 | mask working |
| `NOISE_TOO_HIGH` | ~128 | almost all in the first ~12 min, then silent for ~14 h |
| `LIGHTNING` | 8 | see below |
| `UNCLASSIFIED` | 3 | IRQ fired, reason register read back `0x0` — a known race, not strikes |

The 8 `LIGHTNING` events fell in three time clusters, not scattered:

- ~4 h 42 m – 4 h 52 m: 4 strikes, km 8/6/6/6, energy 18506 / 174784 / 13731 / 40515
- ~9 h 36 m: 1 strike, km 8, energy 105499
- ~10 h 10 m – 10 h 16 m: 3 strikes, km 1/1/1, energy 129286 / 33743 / 31299

The clustering and the spread in distance/energy match real storm cells
passing, not the clear-sky interference signature (fixed distance,
clustered energy, no thunder) the tuning was built to reject. So the
tuning is not producing false strikes. The concern is the opposite:
**8 confirmed strikes across ~15 h of active storms is far fewer than the
storms actually delivered.** Because the monitor services the IRQ
continuously (no deep sleep), the deep-sleep read path is not the cause —
the chip itself only classified 8 events as lightning.

Leading hypotheses, most likely first:

1. `AS3935_MIN_STRIKES = 5` — the chip suppresses the interrupt until it
   has counted 5 strikes inside its ~17-minute window, and the counter
   decays. A storm whose strikes arrive sparser than that at the sensor
   never trips it.
2. `maskDisturber(true)` — some genuine distant strikes are classified as
   disturbers and then suppressed.
3. `watchdogThreshold` / `spikeRejection` at 3 rather than the power-on 2
   reject more borderline real strikes.
4. Indoor placement inside the wooden enclosure attenuating the signal.

### Plan for the next storm

Re-run the monitor during the next thunderstorm with the noise-rejection
knobs loosened, to see how many more strikes the chip reports and whether
interference returns. The `[env:as3935_monitor_loose]` PlatformIO
environment is prepared for exactly this — it is `as3935_monitor` plus
command-line overrides:

| Knob | Normal | Loose env |
|---|---|---|
| `AS3935_MIN_STRIKES` | 5 | 1 |
| `AS3935_WATCHDOG_THRESHOLD` | 3 | 2 |
| `AS3935_SPIKE_REJECTION` | 3 | 2 |
| disturber mask | on | **off** (rate visible) |

The three `config.h` knobs are wrapped in `#ifndef` and the mask is the
`AS3935_MASK_DISTURBER` define, so the loose build changes nothing in
`config.h` and the production firmware is unaffected.

```sh
pio run -e as3935_monitor_loose -t upload   # binary is pre-built; this just flashes
```

Then leave the board on a bare USB charger (no host — a host raises the
noise rate) for the duration of the storm and read `/as3935.log` back
afterwards. Compare the `LIGHTNING` count and the `NOISE_TOO_HIGH` /
`DISTURBER` rates against the 2026-08-31 run before deciding which
loosening to fold into `config.h`. Flash `esp32-s3-devkitc-1` to return
to the production firmware.

## Files

| File | Purpose |
|---|---|
| `lib/as3935_lightning/` | AS3935 bus/IRQ/read wrappers, `dumpAs3935Config()` |
| `src/local_sensors.cpp` | Rate buffer, strike history, stale reset, overhead guard |
| `src/time_manager.cpp` | NTP sync with the `NTP_EPOCH_PLAUSIBLE_MIN` floor |
| `include/config.h` | All `LIGHTNING_*` / `AS3935_*` / `STRIKE_*` constants |
| `src/test_as3935_monitor.cpp` | `as3935_monitor` diagnostic sketch |
