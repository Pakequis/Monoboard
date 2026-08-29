#ifndef LOCAL_SENSORS_H
#define LOCAL_SENSORS_H

#include <Arduino.h>

// Initializes all local sensors (DHT22, AS3935). Call once from setup(),
// before the first readLocalSensors() call.
void initLocalSensors();

// Reads local sensors that are safe to poll every normal wake (currently
// just DHT22). Lightning strikes are captured via the AS3935's IRQ wake
// instead (see handleLightningIrqWake()) -- polling the interrupt
// register on the 25s timer would miss most real strikes (the datasheet
// gives only a ~1s window to read a strike's interrupt register).
void readLocalSensors();

// Fills outText with temperature and humidity only, e.g. "22.5 C 45%"
// (the space between the temperature number and "C" is deliberate — it
// reserves room for the degree circle drawn separately by the caller,
// since this font has no glyph for the actual degree sign). Buffer must
// be at least TEMP_HUMIDITY_TEXT_LEN bytes. Reflects whatever
// readLocalSensors() last read.
void getTempHumidityText(char* outText, size_t outTextSize);

// Fills outText with just the temperature's numeric portion (e.g. "22.5"
// or "--" if unavailable), no unit. Lets the caller measure its pixel
// width to position a drawn degree circle right after it.
void getTemperatureNumberText(char* outText, size_t outTextSize);

// Fills outText with the confirmed-lightning-strike count in the trailing
// LIGHTNING_RATE_WINDOW_SEC (a real sliding window, e.g. "3" meaning 3
// strikes in the last hour -- not a since-last-reset cumulative total),
// shown in the "Raios" header. Writes STR_VALUE_PLACEHOLDER ("--") if the
// AS3935 isn't detected or the clock hasn't synced yet (a time window is
// meaningless without a trustworthy clock). Buffer must be at least
// LIGHTNING_STATUS_TEXT_LEN bytes.
void getLightningRateText(char* outText, size_t outTextSize);

// Returns how many of the last STRIKE_HISTORY_COUNT confirmed strikes are
// currently valid (0..STRIKE_HISTORY_COUNT). Applies the reset-if-stale
// check first (STRIKE_RESET_TIMEOUT_SEC of no strikes clears this history).
uint8_t getStrikeHistoryCount();

// Fills outKm/outEnergy for strike history slot `index`, ordered oldest
// (index 0) to most recent (index getStrikeHistoryCount()-1). Caller must
// only pass index < getStrikeHistoryCount() (call that first).
void getStrikeHistoryEntry(uint8_t index, uint8_t* outKm, uint32_t* outEnergy);

// Records a confirmed lightning strike: applies the reset-if-stale check,
// then writes (km, energy) into the history ring buffer and the strike's
// timestamp into the rate window's buffer. Called from
// handleLightningIrqWake() on a real AS3935 IRQ wake.
void onConfirmedLightningStrike(int km, uint32_t energy);

// Called from main.cpp's short-circuit path when the current wake was
// caused by the AS3935's IRQ pin. Initializes the AS3935 over I2C, reads
// its interrupt register, and if (and only if) it's a confirmed LIGHTNING
// event, records it via onConfirmedLightningStrike(). Safe to call
// whether or not the sensor is present. Touches nothing else (no display,
// no WiFi, no DHT22).
void handleLightningIrqWake();

// Returns true (once) if the strike just recorded by this same boot's
// onConfirmedLightningStrike() call was close (distance at or within
// LIGHTNING_ALERT_KM, energy irrelevant), AND no other close strike has
// already consumed this latch since the last normal-cadence redraw.
// Marks the latch used as a side effect of returning true, so a burst of
// several close strikes only forces one early redraw -- the rest just
// update the alert flag/history and wait for the next redraw, whichever
// triggers it. Call resetCloseStrikeRedrawLatch() whenever a redraw
// happens through the normal cadence instead, to re-arm it.
bool shouldForceEarlyRedraw();

// Re-arms the early-redraw latch (see shouldForceEarlyRedraw() above).
// Call this whenever a redraw is about to happen through the normal
// cadence (a genuine timer wake, or an IRQ wake where a redraw was
// already due anyway) rather than the forced-early exception.
void resetCloseStrikeRedrawLatch();

// Returns whether a close strike (see shouldForceEarlyRedraw()) has been
// recorded since the last time the dashboard was drawn -- drives the
// lightning-bolt alert icon in the lightning box header.
bool isLightningAlertActive();

// Clears the alert-icon flag above. Called once after each dashboard
// redraw so the icon shows for exactly that one redraw, unless a new
// close strike reactivates it before the next one.
void clearLightningAlert();

#endif // LOCAL_SENSORS_H
