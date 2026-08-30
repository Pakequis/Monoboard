/*
  Project Configuration
  Centralized constants to avoid magic numbers
*/

#ifndef CONFIG_H
#define CONFIG_H

// ===== WiFi Configuration =====
// defines WIFI_SSID / WIFI_PASSWORD and optionally WEATHER_CITY_LABEL /
// WEATHER_LATITUDE / WEATHER_LONGITUDE, gitignored; optional include so the
// fallbacks below still apply when the file doesn't exist
#if __has_include("secrets.h")
#include "secrets.h"
#endif
#ifndef WIFI_SSID
#define WIFI_SSID     "dummy-ssid"
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "dummy-password"
#endif
#include "strings.h" // defines APP_LANGUAGE, consumed below by NEWS_API_URL
#define WIFI_TIMEOUT_MS 30000UL // 30 seconds -- observed occasional associations taking close to the old 20s
#define WIFI_FAST_RECONNECT_TIMEOUT_MS 6000UL // give up on the known-BSSID/channel fast path and fall back to a full scan if it hasn't connected by then
#define STATUS_POLL_INTERVAL_MS 500UL // polling interval while waiting for WiFi/NTP

// ===== NTP Configuration =====
#define NTP_SERVER          "pool.ntp.org"
#define GMT_OFFSET_SEC      (-3UL * 60UL * 60UL) // UTC-3 (Brazil/Sao Paulo)
#define DAYLIGHT_OFFSET_SEC 0
#define NTP_SYNC_TIMEOUT_MS 8000UL   // 8 seconds -- matches WEATHER/NEWS_HTTP_TIMEOUT_MS; the old 5s failed more often than those

// ===== Deep Sleep Configuration =====
#define DEEP_SLEEP_INTERVAL_SEC 60UL // Wake up every 60 seconds

// ===== NTP Resync Configuration =====
// Kept short since the RTC's internal RC oscillator drifts noticeably
// without an external 32kHz crystal; cheap now that WiFi only connects
// when a resync is actually due (see isTimeSyncDue()).
#define NTP_RESYNC_INTERVAL_MS (1UL * 60UL * 60UL * 1000UL) // 1 hour

// An epoch below this is treated as "unset" (RTC not yet synced via NTP)
#define NTP_EPOCH_VALID_THRESHOLD 100000

// Sanity floor for an NTP result: 2025-01-01 00:00:00 UTC, comfortably
// before this firmware existed. A sync that comes back below this is a
// bad server response (pool.ntp.org occasionally hands out garbage) and
// is rejected rather than written to the clock -- a wrong "now" that is
// still above NTP_EPOCH_VALID_THRESHOLD would otherwise pass every
// downstream validity check and, worse, make already-stored strike
// timestamps look like they are in the future (see getLightningRateText).
#define NTP_EPOCH_PLAUSIBLE_MIN 1735689600

// ===== Display Configuration =====
#define DISPLAY_FRAME_MARGIN    5   // pixels
#define DISPLAY_RESET_PULSE_MS  10UL // hardware reset pulse width, and GxEPD2's own internal reset duration
#define DISPLAY_BORDER_OFFSET   1   // pixels from the physical screen edge to the outer border rectangle

// ===== Header Layout =====
// Single line: the title stays on the left while WiFi status and firmware
// version are right-aligned on HEADER_TITLE_Y. The clock lives in the bottom-right quadrant instead
// (see CLOCK_LINE_Y_OFFSET below), and there's no second header line for a
// date or status. The freed height goes to the content grid's bottom
// row (see CONTENT_MID_Y's comment).
#define HEADER_TITLE_Y          16
#define HEADER_SEPARATOR_Y      22
#define HEADER_TEXT_X_OFFSET    5   // horizontal inset beyond DISPLAY_FRAME_MARGIN

// ===== Serial Configuration =====
#define SERIAL_BAUD_RATE 115200

// 1 = development build (serial output on), 0 = final/production build
// (serial fully compiled out, including Serial.begin()). Override without
// editing this file via a build flag, e.g. -D APP_DEBUG_SERIAL=0.
#ifndef APP_DEBUG_SERIAL
#define APP_DEBUG_SERIAL 0
#endif

// 1 = on boot, ignore the normal dashboard and instead draw the
// screen-measurement reference (cm rulers on every edge + a 10 cm
// calibration bar), latch it on the e-paper, and deep-sleep forever --
// for physically templating an enclosure. 0 = normal firmware. The code
// is always compiled in; this only gates whether main.cpp reaches it.
// Override without editing this file: -D SHOW_SCREEN_RULER=1 (the
// screen_ruler PlatformIO env does exactly that). Return to normal by
// reflashing with this at 0.
#ifndef SHOW_SCREEN_RULER
#define SHOW_SCREEN_RULER 0
#endif

// ===== Firmware =====
#define FIRMWARE_VERSION "v1.0.0"

// ===== Local Sensor Pins =====
// Avoid GPIO 4/10/11/12/16/17 (committed to the e-paper display wiring),
// GPIO 19/20 (native USB), and GPIO 0/3/45/46 (strapping pins). All
// assignments below stay clear of those.
#define PIN_DHT22_DATA      2   // digital, needs a pull-up (external or on-board)
#define PIN_AS3935_IRQ      6   // deep-sleep wake source (ext1, as3935_lightning.cpp)
                                // -- the ESP32 wakes the instant the chip
                                // signals an event, instead of polling the
                                // interrupt register on the 25s timer (which
                                // missed most real strikes: the datasheet
                                // gives only a ~1s window to read a strike's
                                // interrupt register before it's lost)
#define PIN_AS3935_SDA      8   // I2C data (matches ESP32-S3 Arduino core default)
#define PIN_AS3935_SCL      9   // I2C clock (matches ESP32-S3 Arduino core default)

// Antenna tuning capacitance for this specific AS3935 board, in pF (0-120,
// steps of 8). Measured once via an IRQ-pin pulse-count sweep across all 16
// steps -- 0pF gave the closest match to the 500kHz antenna target
// (496480Hz measured, 0.7% off, well within the datasheet's 3.5% tolerance),
// so no extra capacitance is needed on this board. Re-measure if the sensor
// is ever swapped for a different physical unit.
#define AS3935_TUNE_CAP     0

// ---- AS3935 detection sensitivity ----
// The chip powers on with every noise-rejection knob at its most
// permissive setting, and the SparkFun library never changes them. Left
// that way, a single indoor electrical transient (a brushed motor, an
// SMPS, a car ignition nearby) is enough to raise a LIGHTNING interrupt.
// A full hour of monitoring under a clear sky logged 31 such "strikes"
// in two bursts, distance marching 10 km -> 1 km with energy up to
// ~500000 -- the classic signature of local interference, not weather.
// These override the power-on defaults in initAs3935().
//
// AS3935_MIN_STRIKES: confirmed strikes the chip must see within its
// ~17-minute internal window before it fires the interrupt. Only 1, 5, 9
// or 16 are valid. 5 discards a lone transient while still catching a
// real storm (which produces strikes continuously) with little delay.
#define AS3935_MIN_STRIKES         5
// AS3935_WATCHDOG_THRESHOLD (0-10) and AS3935_SPIKE_REJECTION (0-11):
// higher rejects more non-lightning waveforms, at the cost of also
// missing weak/distant real strikes. One step above the power-on 2 each,
// a conservative first tightening -- raise further if false strikes
// persist, lower toward the defaults if real nearby storms get missed.
#define AS3935_WATCHDOG_THRESHOLD  3
#define AS3935_SPIKE_REJECTION     3
// AS3935_NOISE_LEVEL (1-7): the chip's noise-floor reference; higher
// tolerates a noisier RF environment before raising NOISE_TOO_HIGH.
// Kept at the power-on 2 -- monitoring logged zero NOISE_TOO_HIGH
// events, so the noise floor is not the problem here.
#define AS3935_NOISE_LEVEL         2

// Firmware-side backstop, applied in onConfirmedLightningStrike(): the
// chip reports distance as a running minimum over its event window, so
// one strong local transient pins it at 1 km and later events hold it
// there. A confirmed "strike" at or inside this distance whose energy
// exceeds AS3935_OVERHEAD_MAX_PLAUSIBLE_ENERGY is discarded rather than
// counted -- real overhead strikes on this sensor read energy ~17000,
// far below this ceiling. Set the energy ceiling to 0 to disable the
// check.
#define AS3935_OVERHEAD_SANITY_KM             1
#define AS3935_OVERHEAD_MAX_PLAUSIBLE_ENERGY  200000UL

// ===== Content Feature Flags =====
#define FEATURE_WEATHER_ENABLED 1
#define FEATURE_NEWS_ENABLED    1
#define FEATURE_CRYPTO_ENABLED  1

// ===== Content Area Layout: 2x2 grid =====
// Four quadrants (weather forecast grid, lightning distance box, and
// calendar on top; news headlines and the clock+reserved box on the
// bottom), each its own bordered box -- except the bottom-right
// quadrant, which is itself split into two stacked boxes (clock, then
// a reserved box, see CLOCK_BOX_HEIGHT below). No standalone divider
// lines anywhere in the grid, separation comes entirely from each
// box's own border plus a CONTENT_BOX_MARGIN gap to its neighbors
// (both axes). The box positions are deliberately asymmetric on both
// axes -- tuned against a hand-drawn paper mockup held up to the
// webcam before ever touching these numbers on the physical display.
//
// The header's one-line height (HEADER_SEPARATOR_Y) determines
// topRowY0 -- top-row boxes (weather/lightning/calendar) keep a fixed
// height, so this constant is what actually controls how much of the
// screen height the bottom row (news / clock+reserved) gets.
#define CONTENT_MID_Y          242  // top row vs bottom row boundary -- not drawn itself, just the Y anchor both rows' boxes inset CONTENT_BOX_MARGIN from
// Every quadrant's edges are derived directly from its neighbors'
// geometry instead of a shared divider constant. Bottom-right (clock+
// reserved) boxes' shared left edge/width match the calendar's own
// (`calX0`/`CALENDAR_WIDTH`) since they sit directly under it, same
// width -- a hardcoded twin constant would drift out of sync if
// CALENDAR_WIDTH ever changed.
#define CONTENT_TEXT_X_OFFSET  5    // left offset for content text, beyond a quadrant's own box border
#define CONTENT_BOX_MARGIN     8    // gap from a quadrant's shared boundary (the display frame margin or an adjacent box) to its own box border

// ---- Top row, box 1 (leftmost): weather forecast grid + sensor temp/humidity ----
// 3 columns x 2 rows, with a temp/humidity header bar above the grid,
// drawn white-on-black matching the calendar/lightning header bars.
// Left-anchored; the lightning box's own left edge is derived from
// this box's real width (see dashboard_manager.cpp), so this one stays
// fixed at the frame margin.
#define WEATHER_GRID_COLUMNS         3
#define WEATHER_GRID_ROWS            2
#define WEATHER_BOX_COLUMN_WIDTH     68  // px per hour rectangle
#define WEATHER_BOX_GAP              6   // px gap between adjacent rectangles, both axes
#define WEATHER_SENSOR_LINE_HEIGHT   22  // px, temp/humidity header bar
#define WEATHER_SENSOR_LINE_Y_OFFSET 16  // baseline y offset from the header bar's top
#define WEATHER_HOUR_LABEL_Y_OFFSET  14  // baseline y offset from a rectangle's top, for the hour label
#define WEATHER_TEMP_LABEL_Y_OFFSET  32  // baseline y offset from a rectangle's top, for the temperature label
#define WEATHER_ICON_Y_OFFSET        75  // baseline y offset from a rectangle's top, for the icon

// ---- Top row, box 2 (middle): lightning ("Raios") distance box ----
// Distance scale: 4 dashed rings, adaptive range (see
// dashboard_manager.cpp). Normal mode: rings at 10/20/30/40 km. Close
// mode (every strike in history within LIGHTNING_CLOSE_KM_THRESHOLD):
// rings at 4/6/8/10 km, spaced to spread a close storm's strikes across
// the box (6/8/10 are real AS3935 distance-table entries), each ring's
// radius proportional to its km value. A single strike beyond
// LIGHTNING_CLOSE_KM_THRESHOLD snaps the ruler back to normal. Confirmed
// strikes overlay as solid, thick rings at their real distance -- or a
// filled center disc for an "overhead" reading (see LIGHTNING_OVERHEAD_KM).
// See STRIKE_HISTORY_COUNT/LIGHTNING_ENERGY_* below. Header shows the
// strike rate (getLightningRateText(), LIGHTNING_RATE_WINDOW_SEC below).
// This box's width is whatever's left between the weather grid and the
// calendar (both fixed-width) -- it self-adjusts, no hardcoded width.
#define LIGHTNING_RING_COUNT            4   // concentric rings drawn below the header bar
#define LIGHTNING_MAX_KM                40  // AS3935's own max reported distance (datasheet distance table)
#define LIGHTNING_DASH_LENGTH           4   // px, dashed scale-ring arc length
#define LIGHTNING_DASH_GAP              3   // px, dashed scale-ring gap length
#define LIGHTNING_RING_LABEL_ANGLE_DEG  (-45) // degrees, direction from center each km label sits along (up-and-right)
#define LIGHTNING_HEADER_HEIGHT         22  // px, status bar (drawn white-on-black)
#define LIGHTNING_HEADER_LABEL_Y_OFFSET 16  // baseline y offset from the box top, for the status label
#define LIGHTNING_UNIT_LABEL_X_OFFSET   4   // px, left inset from the box border for the "km" unit label
#define LIGHTNING_UNIT_LABEL_Y_OFFSET   4   // px, upward inset from the box's bottom border (baseline)
#define LIGHTNING_RING_LABEL_GAP_PX     30  // px, arc-length left clear (at LIGHTNING_RING_LABEL_ANGLE_DEG) so a
                                         // strike ring never overwrites a ruler number label
// Strikes closer than this draw a full circle instead of a gapped arc
// (may overlap the 10km ring's own label -- accepted, since a full
// circle reads as "very close" more clearly than a gap would).
#define LIGHTNING_CLOSE_KM_THRESHOLD    10
// Minimum on-screen radius (px) for a confirmed-strike overlay ring. The
// linear km->radius mapping otherwise collapses a nearby strike to ~1px,
// hidden under the center dot -- exactly the strikes that matter most.
// Clamped so any strike from LIGHTNING_CLOSE_KM_THRESHOLD inward still
// reads as a distinct ring.
#define LIGHTNING_STRIKE_MIN_R          10
// A distance-table reading at or below this (km) is the AS3935's "storm
// overhead" code (register value 1). Drawn as a filled disc at the box
// center instead of a ring (a ring would collapse under the center
// marker), and always raises the proximity alert regardless of energy.
#define LIGHTNING_OVERHEAD_KM           1
#define LIGHTNING_OVERHEAD_DISC_R       6   // px, filled-disc radius for an overhead strike
// Proximity alert: the lightning-bolt icon in the "Raios" header, plus
// one forced early redraw per burst (see local_sensors.h's
// shouldForceEarlyRedraw()), fires whenever a confirmed strike lands at
// or within this distance (km), independent of its energy -- the
// AS3935's energy value is not a reliable proxy for "dangerously close"
// (real overhead strikes were observed at energy ~17000, far below
// LIGHTNING_ENERGY_HIGH_THRESHOLD).
#define LIGHTNING_ALERT_KM             5
#define LIGHTNING_ALERT_ICON_SIZE       14  // px, square bounding box
#define LIGHTNING_ALERT_ICON_MARGIN    6   // px, gap from the header bar's right edge

// ---- Strike history: last STRIKE_HISTORY_COUNT confirmed LIGHTNING events ----
#define STRIKE_HISTORY_COUNT 5
// 2 hours of no confirmed strike clears the distance-ring history (the
// visual overlay on the "Raios" box's scale). The header's own count
// (LIGHTNING_RATE_WINDOW_SEC below) is a real trailing window that should
// self-expire, but resetStrikeStateIfStale() clears its buffer on this
// same timeout too, as a backstop for entries stuck in the window by a
// clock that later moved backward. Wrapped in #ifndef so a
// verification build can override it to a short value (e.g.
// -D STRIKE_RESET_TIMEOUT_SEC=8UL) without editing this file, same pattern
// as APP_DEBUG_SERIAL above.
#ifndef STRIKE_RESET_TIMEOUT_SEC
#define STRIKE_RESET_TIMEOUT_SEC (2UL * 60UL * 60UL)
#endif
// The AS3935's energy value has no defined physical unit (datasheet: "a
// pure value that doesn't have any physical meaning") -- these only pick
// the strike ring's thickness tier (thin/medium/thick), nothing else.
// The proximity alert is distance-only (LIGHTNING_ALERT_KM above), not
// energy-gated: real overhead strikes were observed at energy ~17000,
// far below HIGH. Still a rough guess for the thickness tiers -- retune
// once more real samples across a range of distances are collected.
#define LIGHTNING_ENERGY_MEDIUM_THRESHOLD  50000UL
#define LIGHTNING_ENERGY_HIGH_THRESHOLD    300000UL

// ---- Lightning rate: strikes in the trailing LIGHTNING_RATE_WINDOW_SEC ----
// Shown in the header instead of a since-last-reset cumulative count --
// a real sliding window communicates recent activity better than a
// total that only resets after 4h of silence.
#define LIGHTNING_RATE_WINDOW_SEC   3600UL
// Cap on strike timestamps kept for the rate window. If more than this
// many strikes land within one window, the oldest ones are evicted and
// the reported rate undercounts. Raised from 40 after a real storm
// pinned the header at "40/h" for a sustained stretch -- clearly
// undercounting. Still well under 1KB of RTC_DATA_ATTR memory at one
// time_t per slot; the hard ceiling is 255 since strikeRateIndex /
// strikeRateValidCount are uint8_t.
#define LIGHTNING_RATE_MAX_SAMPLES  120

// ---- Top row, box 3 (rightmost): calendar ----
// Right-anchored (unlike before, when it was left-anchored right after
// the old divider) since it's now the last box in the row -- guarantees
// the same DISPLAY_FRAME_MARGIN/CONTENT_BOX_MARGIN gap from the right
// edge regardless of the other boxes' widths.
#define CALENDAR_WIDTH          182 // px (26px/column -- 140 made 2-digit days overlap their neighbors, since "27" alone is ~22px wide at this font size)
#define CALENDAR_HEADER_HEIGHT  22  // px, month/year bar (drawn white-on-black, "negativo")
#define CALENDAR_WEEKDAY_HEIGHT 18  // px, weekday-letter row below the header bar
#define CALENDAR_WEEK_ROWS      5   // a month never spans more than 5 calendar rows
#define CALENDAR_TITLE_LABEL_Y_OFFSET   16 // baseline y offset from the box top, for the month/year text
#define CALENDAR_WEEKDAY_LABEL_Y_OFFSET 13 // baseline y offset from CALENDAR_HEADER_HEIGHT, for the weekday letters
#define CALENDAR_DAY_LABEL_Y_OFFSET     21 // baseline y offset from a week row's top, for the day number

// ---- Bottom-right quadrant: clock box + crypto/FX box ----
// Two separate bordered boxes stacked with the usual CONTENT_BOX_MARGIN
// gap between them (same pattern as every other quadrant boundary in
// this grid), instead of one box holding both. Clock box on top, in
// lib/dseg_fonts/dseg7_classic_bold.h's DSEG7_Classic_Bold_44. The box
// below it shows 3 plain text lines (BTC, ETH, FX rate -- see the Crypto/
// FX Content Type section below), same left-aligned style as the news
// column alongside it, no header bar (box is too short to spare the
// height). Quadrant sits directly under the calendar, same width (see
// CONTENT_MID_Y's comment above).
#define CLOCK_LINE_Y_OFFSET  52  // baseline y offset from the clock box's own top, clock line: an 8px top margin, plus the DSEG7 font's own 44px ascender (top edge to baseline)
#define CLOCK_BOX_BOTTOM_MARGIN 8  // px, gap below the clock's baseline to the clock box's own bottom edge -- mirrors CLOCK_LINE_Y_OFFSET's 8px top margin
#define CLOCK_BOX_HEIGHT (CLOCK_LINE_Y_OFFSET + CLOCK_BOX_BOTTOM_MARGIN)  // px, clock-only box height: top margin + DSEG7 ascender + bottom margin
#define CRYPTO_FIRST_LINE_Y_OFFSET 16  // baseline y offset from the crypto box's own top, for the BTC line -- matches WEATHER_SENSOR_LINE_Y_OFFSET's single-line pitch
#define CRYPTO_LINE_HEIGHT         20  // baseline-to-baseline pitch between the BTC/ETH/FX lines -- matches NEWS_LINE_HEIGHT

// ---- Bottom-left quadrant: news headlines ----
// No header label (unlike weather/lightning/calendar above, though the
// clock and reserved boxes alongside it skip one too) -- the list of
// headlines is self-explanatory. Each headline wraps across up to 2 lines
// (word-wrapped, not just truncated) instead of 1 -- most real headlines
// don't fit this font's ~36 chars/line. Only NEWS_HEADLINES_PER_BUFFER (3)
// show on screen at once, at a line pitch proven comfortable elsewhere in
// this codebase (the weather grid's hour/temp lines). A headline short
// enough to need only 1 line just leaves its second line slot blank.
//
// Carousel: NEWS_TOTAL_HEADLINE_COUNT (9) headlines are fetched and
// cached per sync window, grouped into NEWS_CAROUSEL_BUFFER_COUNT (3)
// buffers of NEWS_HEADLINES_PER_BUFFER (3) each. Every screen redraw
// (not every sync -- see advanceNewsCarousel(), called once per wake from
// main.cpp) shows the next buffer, cycling back to the first after the
// last -- so all 9 headlines get shown 3 at a time across 3 consecutive
// wakes, instead of only ever showing the same first 3.
#define NEWS_FIRST_LINE_Y_OFFSET 24  // baseline y offset from CONTENT_MID_Y, for headline 0's first line
#define NEWS_LINE_HEIGHT         20  // baseline-to-baseline pitch, both within a headline's 2 lines and between headlines -- uses part of the extra height freed by the one-line header instead of cramming in more headlines

// ===== Degree Symbol (temperature text) =====
// FreeMonoBold9pt7b has no glyph for U+00B0 ('°'), so the degree mark is
// drawn as a small circle instead of rendered as text. Positioned right
// after the temperature number's ink width, within the blank space
// reserved before "C" (see getTempHumidityText()).
#define DEGREE_CIRCLE_RADIUS  2  // pixels
#define DEGREE_CIRCLE_X_OFFSET 9 // gap from the temperature number's ink width to the circle center
#define DEGREE_CIRCLE_Y_OFFSET 8 // pixels above the sensor strip text baseline

// ===== Content Cache / History =====
#define TEMP_HUMIDITY_TEXT_LEN      16  // fixed buffer size for the temp/humidity text (e.g. "22.5 C 45%")
#define LIGHTNING_STATUS_TEXT_LEN   10  // fixed buffer size for the lightning status label (e.g. "40km", "--")
#define CRYPTO_LINE_TEXT_LEN        20  // fixed buffer size per crypto/FX box line (e.g. "BTC R$304521", "BRL $0.19")

// ===== Weather Forecast Content Type =====
// Open-Meteo needs lat/long, not a city name -- WEATHER_CITY_LABEL is just a
// human-readable note, geocoded once via
// https://geocoding-api.open-meteo.com/v1/search?name=<city>&count=1 and
// never used in the actual API call. forecast_hours in the URL is a
// separate literal from WEATHER_FORECAST_HOURS below (no preprocessor
// stringification here) -- keep both in sync by hand if this ever changes.
// The reference city lives in secrets.h (gitignored) so it isn't hardcoded
// here; if secrets.h is absent, or doesn't define these, fall back to Sao Paulo.
#ifndef WEATHER_CITY_LABEL
#define WEATHER_CITY_LABEL      "Sao Paulo, SP" // ASCII-only per this project's display-string convention
#endif
#ifndef WEATHER_LATITUDE
#define WEATHER_LATITUDE        "-23.5505"
#endif
#ifndef WEATHER_LONGITUDE
#define WEATHER_LONGITUDE       "-46.6333"
#endif
#define WEATHER_FORECAST_HOURS  6 // cards displayed in the dashboard
// One extra interval is fetched because the interval currently in progress
// is skipped; the display then contains the next six complete hours.
#define WEATHER_API_URL         "https://api.open-meteo.com/v1/forecast?latitude=" WEATHER_LATITUDE "&longitude=" WEATHER_LONGITUDE "&hourly=weathercode,temperature_2m,is_day&forecast_hours=7&timezone=America%2FSao_Paulo"
#define WEATHER_HTTP_TIMEOUT_MS 8000UL

// ===== News Headlines Content Type =====
// Google News' "top stories" RSS feed, no API key needed. The locale
// query params (hl/gl/ceid) come from strings.h so the fetched language
// follows APP_LANGUAGE instead of always being Brazilian Portuguese.
// The URL below is the canonical post-redirect form (ceid's language half
// uses the 3-digit variant, e.g. pt-419) -- hitting it directly avoids
// relying on the HTTP client to follow a 302.
#define NEWS_API_URL            "https://news.google.com/rss?" STR_NEWS_LOCALE_QUERY
#define NEWS_HTTP_TIMEOUT_MS    8000UL
#define NEWS_HEADLINES_PER_BUFFER   3   // how many headlines show on screen at once (kept low enough to fit 2 full lines per headline instead of 1 truncated line)
#define NEWS_CAROUSEL_BUFFER_COUNT  3   // how many buffers of NEWS_HEADLINES_PER_BUFFER rotate through, one per screen redraw
#define NEWS_TOTAL_HEADLINE_COUNT   (NEWS_HEADLINES_PER_BUFFER * NEWS_CAROUSEL_BUFFER_COUNT)  // fetched/cached per sync window
#define NEWS_HEADLINE_LEN       96  // fixed buffer size per cleaned-up headline (pre pixel-width truncation, done at render time)
#define NEWS_SOURCE_TAG         "[GN]" // language-independent abbreviation, not translated text

// ===== Crypto/FX Content Type =====
// CoinGecko's simple/price endpoint, no API key needed. One request
// covers both coins and both currencies -- the USD/BRL exchange rate
// shown alongside BTC/ETH isn't a separate fetch, it's derived from
// these same four prices at render time (see crypto_client.h). Which
// currency each price line displays in (BRL vs USD) follows APP_LANGUAGE
// via strings.h's STR_CRYPTO_* macros, not a separate flag here.
#define CRYPTO_API_URL           "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin,ethereum&vs_currencies=usd,brl"
#define CRYPTO_HTTP_TIMEOUT_MS   8000UL
#define CRYPTO_BTC_LABEL         "BTC" // ticker symbol, language-independent like NEWS_SOURCE_TAG above
#define CRYPTO_ETH_LABEL         "ETH"

#endif // CONFIG_H
