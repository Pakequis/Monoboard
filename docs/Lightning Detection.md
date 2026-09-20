# Lightning Detection

> How the SparkFun AS3935 is driven: the deep-sleep IRQ wake, the
> strikes/hour header metric, the `OUTDOOR` front-end gain that made the
> sensor actually detect storms, the sensitivity tuning that keeps local
> electrical noise from registering as strikes, and the `as3935_monitor`
> diagnostic build.

Wiring and the breakout's fixed straps are in `Pin Mapping.md`. Antenna
tuning (`AS3935_TUNE_CAP`) is `0` pF for this physical unit.

**If you read one section, read "Antenna gain: indoor vs outdoor"** —
the chip's `INDOOR` power-on gain made it report three separate storms
as nothing at all; `OUTDOOR` fixed it.

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

## Antenna gain: indoor vs outdoor

`initAs3935()` sets the analog front-end gain via
`setIndoorOutdoor(AS3935_INDOOR_OUTDOOR)`. The chip powers on `INDOOR`
(register `0x12`, gain boost 18) and SparkFun leaves it there; this
project overrides it to `OUTDOOR` (`0x0E`, gain 14).

**Why this is the single most important setting here.** `INDOOR`'s
higher gain is meant to recover signals attenuated by a building, but at
this sensor's location it overdrives the front end: local EM
interference saturates the AFE, the chip's automatic noise floor backs
off, and real sferics arrive distorted enough that the
signal-verification stage classifies them as `DISTURBER` — or discards
them entirely — instead of `LIGHTNING`.

The evidence is unambiguous. In `INDOOR` mode: Run 1 logged only 8
`LIGHTNING` across ~15 h of confirmed storms, and three later runs
against two more storms with audible thunder — Run 2 (every rejection
knob at minimum, disturber mask off) and both Run 3 `INDOOR` phases (one
with the sensor physically outside its wooden enclosure) — logged
**zero** between them, while emitting a steady 30–115 `DISTURBER`/min.
The same board, same placement, switched to `OUTDOOR` in the middle of
the Run 3 storm registered **66 `LIGHTNING` events in 14 minutes**
(distance 6 km → 1 km as the cell closed in, energy 8k–480k) with the
disturber rate collapsing to ~1.7/min. Full numbers in Run 3 below.

`AS3935_INDOOR_OUTDOOR` is `#ifndef`-wrapped; the
`[env:as3935_monitor_outdoor]` build forces `OUTDOOR` explicitly on top
of the loosened knobs, which is what isolated the fix.

The cost of `OUTDOOR` is real but acceptable here: lower gain means a
genuinely weak, distant strike is more likely to be missed. That
trade-off is worth it — a sensor that reports regional storms as nothing
at all is not doing its job, and this location's interference floor left
no room to run `INDOOR`.

## Sensitivity tuning

> The knob values below were chosen against `INDOOR`-mode data, before
> the gain problem above was understood. They still hold — `INDOOR` was
> the bottleneck, not the rejection thresholds (loosening them in
> `INDOOR` mode recovered nothing) — but re-read them with that history
> in mind.

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
| `AS3935_INDOOR_OUTDOOR` | `INDOOR` | **`OUTDOOR`** | Front-end gain. Not a threshold — see the section above; this is the change that made the sensor work. |

Raise the thresholds further if false strikes persist; lower them toward
the defaults if real nearby storms get missed.

### Firmware-side overhead guard

The chip reports distance as a **running minimum** over its event window,
so one strong local transient pins it at 1 km and later events hold it
there. `onConfirmedLightningStrike()` discards a strike at
`<= AS3935_OVERHEAD_SANITY_KM` (1 km) whose energy exceeds
`AS3935_OVERHEAD_MAX_PLAUSIBLE_ENERGY`. Set the energy ceiling to `0` to
disable the check.

The ceiling is **550000** (raised from an `INDOOR`-era 200000 in Run 4).
In `OUTDOOR` mode a real close strike's energy spans the
whole 3k–505k range — 331 strikes across the Run 3 and Run 4
storms, `km=1` maxing at 503121 (485678 in Run 3, close agreement) —
so the old 200000 ceiling was discarding about 4% of real close strikes
and the dashboard header undercounted by that much. 550000 sits just
above the observed real-strike maximum, which makes the guard nearly
inert: it now fires only on a reading past anything a genuine close
strike has produced.

> **Could be retired.** The interference signature this guard was built
> for — a local source pinning `km=1` with very high energy — was only
> ever seen in `INDOOR` mode. Both `OUTDOOR` production-knob storm runs
> logged `disturber=0 noise=0`, so that interference may be gone. It has
> not been removed (`= 0`) yet because that needs a clear-sky `OUTDOOR`
> baseline run to confirm — until then the near-inert 550000 ceiling is
> cheap insurance. If the baseline is clean, set the ceiling to `0` and
> rely on the distance-only proximity alert. If `OUTDOOR` interference
> *also* pins `km=1` at ~500k energy, no ceiling short of that helps and
> the guard should go regardless.

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

### Run 1 — real thunderstorms, count lower than expected

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

### Run 2 — loosened knobs, storm ~20 km, zero lightning

The re-test planned after Run 1: `as3935_monitor_loose`
(`MIN_STRIKES=1`, `watchdog=2`, `spikeRejection=2`, disturber mask
**off**) flashed while a thunderstorm was active ~20 km away — cells over
Cachoeira de Minas / Brazópolis / Paraisópolis, confirmed against an
external lightning map. Board on USB with a host attached (not the bare
charger the earlier run used).

Flash-log segment for the run (`BOOT 5` in `/as3935.log`):

| Class | Count | Notes |
|---|---|---|
| `LIGHTNING` | **0** | none in 20 min of active storm |
| `DISTURBER` | 741 | ~36/min, flat for the whole run |
| `NOISE_TOO_HIGH` | 0 | host attached, still zero |
| `UNCLASSIFIED` | 1 | the `0x0`-readback race again |

A fresh segment captured **after the storm had passed** held the disturber
rate at ~28/min — essentially unchanged. So the disturber stream is
almost entirely persistent local EM interference that the loosened
`watchdog`/`spikeRejection` let through, not the storm; and the storm
itself, at 20 km, produced **not one event the chip was willing to call
lightning**, even with `MIN_STRIKES` at 1 and every rejection knob at its
most permissive.

This confirms and sharpens the Run 1 finding: loosening the
noise-rejection knobs does **not** recover strikes — real lightning is
classified as `DISTURBER` (or nothing) upstream of the count, so no
threshold change downstream can bring it back. The production config
(`MIN_STRIKES=5`, `watchdog=3`, `spikeRejection=3`, mask on) stays as-is;
loosening buys nothing but a flood of disturbers.

> **Follow-up (Run 3): the cause was the front-end gain, not the
> antenna or the thresholds.** This section concluded the fix had to be
> "an antenna/placement problem, not a register-tuning one." Half right:
> it was a register — `setIndoorOutdoor()`. Switching from `INDOOR` to
> `OUTDOOR` gain, same board and placement, took the sensor from zero
> `LIGHTNING` to 66 in 14 minutes. See Run 3 below and the
> "Antenna gain: indoor vs outdoor" section. The "only registers close
> strikes" claim was also an `INDOOR`-mode artifact: the Run 3
> `OUTDOOR` run picked the cell up at 6 km and tracked it in.

Note: the `==== BOOT n … mask=1 ====` header line the monitor writes to
the flash log always prints `mask=1` — it is a literal in the `snprintf`
format, not the live `AS3935_MASK_DISTURBER` value — so it reads `mask=1`
even for the loose build, where the mask is off. The `DISTURBER` counts
in the same segment are the real indicator of whether the mask was on.

### Run 3 — INDOOR→OUTDOOR gain, the fix

A close storm with repeated loud thunder. The sensor was outside its
enclosure this time, on USB with a host attached. The run went in four
phases against the same afternoon's storm (phases 1–3 back to back,
phase 4 after the cell circled back):

1. **`as3935_monitor` (production knobs), INDOOR** — ~23 min, audible
   thunder throughout. `noise=0 disturber=0 lightning=0` — not one event
   of any class. (`MIN_STRIKES=5` accumulation was ruled out: the run
   outlasted the chip's ~17-min window with nothing to show.)
2. **`as3935_monitor_loose` (knobs at minimum, mask off), INDOOR** —
   ~50 min. Still `lightning=0`. `DISTURBER` climbed steadily to ~2900
   (50–115/min), `noise` crept to 9. Same picture as Run 2, now
   without the enclosure as an excuse.
3. **`as3935_monitor_outdoor` (loose knobs + `OUTDOOR` gain)** — the
   only change from phase 2 was `setIndoorOutdoor(INDOOR)` →
   `setIndoorOutdoor(OUTDOOR)`. Detailed below.
4. **`as3935_monitor` (production knobs) + `OUTDOOR`** — after the cell
   circled back, to check the full rejection stack. Detailed in the
   conclusion.

Phase 3, first 14 minutes:

| Class | Count | Notes |
|---|---|---|
| `LIGHTNING` | **66** | first two heartbeats still 0, then a burst |
| `DISTURBER` | 24 | ~1.7/min — down from 50–115/min in phase 2 |
| `NOISE_TOO_HIGH` | 0 | |
| `UNCLASSIFIED` | 0 | |

The strikes tracked the storm honestly: the first cluster read `km=6`,
the next `km=5`, then `km=1` once a close strike landed and the chip's
running-minimum distance pinned there. Energy ranged 8k–480k with no
clustering — the fingerprint of real strikes, the opposite of the
clear-sky interference signature. Two bursts (≈15:15 and ≈15:24) with a
quiet stretch between, matching cells passing.

**Conclusion:** `INDOOR` gain was the whole problem. At this location it
saturates the front end, and the chip throws real sferics away as
disturbers before they can be counted. `OUTDOOR` is now the `config.h`
default (`AS3935_INDOOR_OUTDOOR`).

**Phase 4 — production knobs in `OUTDOOR`, confirmed.** Later the same
afternoon the storm re-intensified. The production firmware
(`esp32-s3-devkitc-1`, now `OUTDOOR` by default) was already running and
its dashboard header climbed to 56 strikes/hour on its own. Reflashing
`as3935_monitor` — full production rejection stack, `MIN_STRIKES=5`,
`watchdog=3`, `spikeRejection=3`, disturber mask **on**, verified by the
boot register readback — logged **92 `LIGHTNING` events in a 5-minute
burst** (≈18/min), then nothing once the cell moved off. All `km=1`,
energy 5.6k–486k, `disturber=0` (mask working), `noise=0`. So the
rejection knobs are not a bottleneck in `OUTDOOR` mode either — the
tightening chosen against `INDOOR` clear-sky data carries over fine.
`INDOOR` gain really was the entire fault.

Open item at the time: the firmware overhead guard's 200000 energy
ceiling was stale for `OUTDOOR` — 10 of the 92 phase-4 strikes read above
it and the production firmware would have dropped them. Addressed in
Run 4 (ceiling raised to 550000 after a second storm confirmed the
distribution); see that run below and the "Firmware-side overhead guard"
section.

### Run 4 — OUTDOOR production knobs, storm overhead, energy distribution

Run to collect a real-strike energy/distance distribution under the exact
production configuration, to calibrate the firmware energy constants
(`AS3935_OVERHEAD_MAX_PLAUSIBLE_ENERGY`, `LIGHTNING_ENERGY_MEDIUM_THRESHOLD`,
`LIGHTNING_ENERGY_HIGH_THRESHOLD`).

Setup: `as3935_monitor` env — full production rejection stack
(`MIN_STRIKES=5`, `watchdog=3`, `spikeRejection=3`, disturber mask **on**,
`OUTDOOR` gain, verified by the flash-log `BOOT` header
`wdth=3 nf=2 srej=3 minstk=5`), board on USB with a host attached. A
strong storm passed directly overhead (loud thunder, heavy rain), then
moved off to ~8 km over ~45 min.

The run spans two flash-log segments — a mains power cut killed the board
mid-storm and it rebooted when power returned (`BOOT 13` ≈ 20 min while
the cell was overhead, `BOOT 14` ≈ 26 min of the tail as it moved off).
Figures below combine both. The host serial capture is preserved under
`docs/temp-storm-run4/` (gitignored); the numbers here are from the
on-chip `/as3935.log`, which is complete across the power cut.

| Class | Count | Notes |
|---|---|---|
| `LIGHTNING` | 331 | ~20/min at the peak (`BOOT 13`), tapering to near zero |
| `DISTURBER` | 0 | mask on — zero across the whole storm |
| `NOISE_TOO_HIGH` | 0 | host attached, still zero |
| `UNCLASSIFIED` | 1 | the `0x0`-readback race |

Distance: 174 events at `km=1`, 152 at `km=8`, 2 at `km=5`, 3 at `km=10`.
The chip's running-minimum distance sat at 1 km while the cell was
overhead, then released to 8 km as it moved off — it tracked the storm
honestly, no stuck-at-1 km artifact.

Energy over all 331 `LIGHTNING` events:

| stat | value |
|---|---|
| min | 3 264 |
| median | 19 957 |
| p75 | 34 966 |
| p90 | 63 216 |
| p95 | 89 061 |
| p99 | 371 996 |
| max | 503 121 |
| mean | 33 845 |

2.1% of all events exceed the current 200 000 overhead-guard ceiling;
0.3% exceed 500 000. The max, 503 121, matches the 485 678 seen in
Run 3 — the AS3935's `OUTDOOR` energy reading for a real strike tops
out around **505k**, and the distribution is a smooth long tail from ~3k
with no clustering (the real-strike fingerprint).

**Overhead-guard impact on production.** The guard only fires at
`km <= AS3935_OVERHEAD_SANITY_KM` (1 km). Of the 174 `km=1` events:

- 7 (**4.0%**) read energy above 200 000 — the production firmware
  discards these
- median `km=1` energy 15 800, p95 185 600, max 503 121

So during this storm the production firmware silently dropped about 1 in
25 of the closest strikes, and the dashboard strikes/hour header
undercounts by that fraction — confirming and quantifying the Run 3
open item, now with production knobs rather than the loose build.

The deeper problem: in `OUTDOOR` mode a real overhead strike's energy
spans the **entire** 3k–500k range, so the guard's original premise —
"real overhead ≈ 17k, interference-driven fake overhead ≈ 500k, reject
the high ones" — no longer separates the two. Any ceiling low enough to
catch fake overhead strikes also discards real ones.

### `config.h` changes applied from this run

1. **`AS3935_OVERHEAD_MAX_PLAUSIBLE_ENERGY`: 200000 → 550000.** Real
   strikes in this run maxed at 503 121, so 550 000 drops none of them
   while still clipping a reading past anything a genuine close strike
   has ever produced. This makes the guard nearly inert — acceptable,
   since production knobs produced `disturber=0 noise=0` through two real
   storms (this run and Run 3 phase 4), i.e. no sign of the
   fake-overhead interference the guard was built for. The guard is kept
   rather than set to `0` because that interference signature was
   characterised in `INDOOR` mode and no clear-sky `OUTDOOR` baseline has
   been captured to confirm the gain change eliminated it. If such a
   baseline later confirms it, retire the guard (`= 0`) and rely on the
   distance-only proximity alert (`LIGHTNING_ALERT_KM`).

2. **`LIGHTNING_ENERGY_MEDIUM_THRESHOLD`: 50000 → 30000** and
   **`LIGHTNING_ENERGY_HIGH_THRESHOLD`: 300000 → 90000**. These only
   pick the strike ring's thickness tier. The old values rendered ~85%
   of strikes "thin" and almost none "thick"; 30k / 90k (≈ p70 / p95 of
   this run's distribution) splits it roughly 70% thin / 25% medium /
   5% thick, so the tiers actually communicate something. The energy
   value has no physical meaning, so this is purely a visual choice.

### Re-running the loosened monitor

Two diagnostic envs sit on top of `as3935_monitor`, each just
command-line overrides:

| Knob | `as3935_monitor` | `_loose` | `_outdoor` |
|---|---|---|---|
| `AS3935_MIN_STRIKES` | 5 | 1 | 1 |
| `AS3935_WATCHDOG_THRESHOLD` | 3 | 2 | 2 |
| `AS3935_SPIKE_REJECTION` | 3 | 2 | 2 |
| disturber mask | on | **off** | **off** |
| `AS3935_INDOOR_OUTDOOR` | `OUTDOOR` (default) | `OUTDOOR` (default) | `OUTDOOR` (explicit) |

The knobs are `#ifndef`-wrapped and the mask/gain are their own defines,
so neither build touches `config.h` and the production firmware is
unaffected. `_outdoor` is `_loose` with `OUTDOOR` pinned explicitly — a
leftover from when `INDOOR` was the default; the two now differ only in
that redundancy.

```sh
pio run -e as3935_monitor_loose -t upload    # loose knobs, OUTDOOR via default
pio run -e as3935_monitor_outdoor -t upload  # same, OUTDOOR pinned
pio run -e as3935_monitor -t upload          # production knobs, OUTDOOR via default
```

Then leave the board on a bare USB charger (no host — a host raises the
noise rate) for the duration of the storm and read `/as3935.log` back
afterwards. Flash `esp32-s3-devkitc-1` to return to the production
firmware.

## Files

| File | Purpose |
|---|---|
| `lib/as3935_lightning/` | AS3935 bus/IRQ/read wrappers, `dumpAs3935Config()` |
| `src/local_sensors.cpp` | Rate buffer, strike history, stale reset, overhead guard |
| `src/time_manager.cpp` | NTP sync with the `NTP_EPOCH_PLAUSIBLE_MIN` floor |
| `include/config.h` | All `LIGHTNING_*` / `AS3935_*` / `STRIKE_*` constants, including `AS3935_INDOOR_OUTDOOR` |
| `src/test_as3935_monitor.cpp` | `as3935_monitor` diagnostic sketch |
| `platformio.ini` | `as3935_monitor`, `_loose`, `_outdoor` diagnostic envs |
