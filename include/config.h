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

// ===== Firmware =====
#define FIRMWARE_VERSION "v1.0.0"

// ===== Local Sensor & Button Pins =====
// Avoid GPIO 4/10/11/12/16/17 (committed to the e-paper display wiring),
// GPIO 19/20 (native USB), and GPIO 0/3/45/46 (strapping pins). All
// assignments below stay clear of those.
#define PIN_REFRESH_BUTTON  1   // ext0 deep-sleep wakeup source, RTC-capable
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

// ===== Content Feature Flags =====
#define FEATURE_WEATHER_ENABLED 1
#define FEATURE_NEWS_ENABLED    1

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
// Real 0-40km distance scale: 4 dashed rings at round-number km values
// (not matched to the chip's 15 discrete distance-table entries), each
// labeled. Confirmed strikes overlay as solid, thick rings at their
// real distance -- see STRIKE_HISTORY_COUNT/LIGHTNING_ENERGY_* below.
// Header shows the strike rate (getLightningRateText(),
// LIGHTNING_RATE_WINDOW_SEC below).
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
// circle reads as "very close" more clearly than a gap would). Also the
// distance half of the strong-nearby-strike redraw/alert trigger below.
#define LIGHTNING_CLOSE_KM_THRESHOLD    10
// Alert icon (lightning bolt) shown in the header when a strike lands
// both above LIGHTNING_ENERGY_HIGH_THRESHOLD and inside
// LIGHTNING_CLOSE_KM_THRESHOLD -- see local_sensors.h's isLightningAlertActive().
#define LIGHTNING_ALERT_ICON_SIZE       14  // px, square bounding box
#define LIGHTNING_ALERT_ICON_MARGIN    6   // px, gap from the header bar's right edge

// ---- Strike history: last STRIKE_HISTORY_COUNT confirmed LIGHTNING events ----
#define STRIKE_HISTORY_COUNT 5
// 2 hours of no confirmed strike clears the distance-ring history (the
// visual overlay on the "Raios" box's scale). The header's own count
// (LIGHTNING_RATE_WINDOW_SEC below) is a real trailing window instead, so
// it self-expires and doesn't need this reset. Wrapped in #ifndef so a
// verification build can override it to a short value (e.g.
// -D STRIKE_RESET_TIMEOUT_SEC=8UL) without editing this file, same pattern
// as APP_DEBUG_SERIAL above.
#ifndef STRIKE_RESET_TIMEOUT_SEC
#define STRIKE_RESET_TIMEOUT_SEC (2UL * 60UL * 60UL)
#endif
// The AS3935's energy value has no defined physical unit (datasheet: "a
// pure value that doesn't have any physical meaning") -- these are a
// starting guess for 3 relative ring-thickness tiers, to retune once real
// strikes are observed.
#define LIGHTNING_ENERGY_MEDIUM_THRESHOLD  50000UL
#define LIGHTNING_ENERGY_HIGH_THRESHOLD    300000UL

// ---- Lightning rate: strikes in the trailing LIGHTNING_RATE_WINDOW_SEC ----
// Shown in the header instead of a since-last-reset cumulative count --
// a real sliding window communicates recent activity better than a
// total that only resets after 4h of silence.
#define LIGHTNING_RATE_WINDOW_SEC   3600UL
// Cap on strike timestamps kept for the rate window. If more than this
// many strikes land within one window, the oldest ones are evicted and
// the reported rate undercounts -- accepted trade-off for a fixed-size
// RTC_DATA_ATTR buffer; 40/h is already an intense storm for a single
// AS3935 sensor.
#define LIGHTNING_RATE_MAX_SAMPLES  40

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

// ---- Bottom-right quadrant: clock box + reserved box ----
// Two separate bordered boxes stacked with the usual CONTENT_BOX_MARGIN
// gap between them (same pattern as every other quadrant boundary in
// this grid), instead of one box holding both. Clock box on top, in
// lib/dseg_fonts/dseg7_classic_bold.h's DSEG7_Classic_Bold_44. The box
// below it is left empty (3 blank line-slots at NEWS_LINE_HEIGHT's
// pitch, matching the news column alongside it) until a use is decided
// for it. Quadrant sits directly under the calendar, same width (see
// CONTENT_MID_Y's comment above).
#define CLOCK_LINE_Y_OFFSET  52  // baseline y offset from the clock box's own top, clock line: an 8px top margin, plus the DSEG7 font's own 44px ascender (top edge to baseline)
#define CLOCK_BOX_BOTTOM_MARGIN 8  // px, gap below the clock's baseline to the clock box's own bottom edge -- mirrors CLOCK_LINE_Y_OFFSET's 8px top margin
#define CLOCK_BOX_HEIGHT (CLOCK_LINE_Y_OFFSET + CLOCK_BOX_BOTTOM_MARGIN)  // px, clock-only box height: top margin + DSEG7 ascender + bottom margin

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

#endif // CONFIG_H
