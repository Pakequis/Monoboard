#include "local_sensors.h"
#include "config.h"
#include "debug.h"
#include "dht22_sensor.h"
#include "as3935_lightning.h"
#include "strings.h"
#include <math.h>
#include <time.h>

static float lastTempC = NAN;
static float lastHumidity = NAN;

// Set fresh every normal-wake boot by initLocalSensors() before it's read
// by getLightningRateText() later in the same boot -- not RTC_DATA_ATTR,
// since it doesn't need to survive across a sleep cycle.
static bool sensorPresentCache = false;

// Ring-buffer strike history + count, RTC_DATA_ATTR so they survive deep
// sleep. strikeHistoryIndex is the NEXT slot to write.
static RTC_DATA_ATTR time_t   lastStrikeEpoch = 0;
static RTC_DATA_ATTR uint8_t  strikeHistoryKm[STRIKE_HISTORY_COUNT] = {0};
static RTC_DATA_ATTR uint32_t strikeHistoryEnergy[STRIKE_HISTORY_COUNT] = {0};
static RTC_DATA_ATTR uint8_t  strikeHistoryIndex = 0;
static RTC_DATA_ATTR uint8_t  strikeHistoryValidCount = 0;

// Separate ring buffer of strike timestamps for the header's rate metric
// (getLightningRateText()) -- decoupled from strikeHistory* above, which
// caps at STRIKE_HISTORY_COUNT (5) for visual reasons unrelated to how
// many strikes actually landed in the last hour.
static RTC_DATA_ATTR time_t  strikeRateTimestamps[LIGHTNING_RATE_MAX_SAMPLES] = {0};
static RTC_DATA_ATTR uint8_t strikeRateIndex = 0;
static RTC_DATA_ATTR uint8_t strikeRateValidCount = 0;

// Latch bounding a burst of strong-nearby strikes to a single forced-
// early redraw (RTC_DATA_ATTR since the burst can span several deep-sleep
// cycles); the alert icon flag, same reason. lastStrikeWasStrongClose is
// NOT RTC_DATA_ATTR on purpose -- it only needs to answer for strikes
// confirmed during this same boot, and must default back to false on
// every fresh boot rather than carry a stale value from a previous one.
static RTC_DATA_ATTR bool strongStrikeRedrawUsed = false;
static RTC_DATA_ATTR bool lightningAlertActive = false;
static bool lastStrikeWasStrongClose = false;

void initLocalSensors()
{
  initDht22();
  sensorPresentCache = initAs3935();
}

void readLocalSensors()
{
  if (!readDht22(&lastTempC, &lastHumidity))
  {
    lastTempC = NAN;
    lastHumidity = NAN;
    DEBUG_PRINTLN("DHT22 read failed");
  }
  else
  {
    DEBUG_PRINT("DHT22 OK - Temp: ");
    DEBUG_PRINT(lastTempC);
    DEBUG_PRINT(" C, Humidity: ");
    DEBUG_PRINT(lastHumidity);
    DEBUG_PRINTLN(" %");
  }
}

void getTemperatureNumberText(char* outText, size_t outTextSize)
{
  if (!isnan(lastTempC))
  {
    snprintf(outText, outTextSize, "%.1f", lastTempC);
  }
  else
  {
    snprintf(outText, outTextSize, "--");
  }
}

void getTempHumidityText(char* outText, size_t outTextSize)
{
  char tempNumBuf[8];
  char tempBuf[10];
  char humBuf[6];

  getTemperatureNumberText(tempNumBuf, sizeof(tempNumBuf));
  snprintf(tempBuf, sizeof(tempBuf), "%s C", tempNumBuf);

  if (!isnan(lastHumidity))
  {
    snprintf(humBuf, sizeof(humBuf), "%.0f%%", lastHumidity);
  }
  else
  {
    snprintf(humBuf, sizeof(humBuf), "--%%");
  }

  snprintf(outText, outTextSize, "%s %s", tempBuf, humBuf);
}

static void resetStrikeHistoryIfStale()
{
  time_t now = time(nullptr);

  // Both epochs must be post-NTP-sync valid before trusting their
  // difference. Without this guard, a strike recorded early in a boot
  // (before that boot's own NTP resync corrects the clock) gets compared
  // against the corrected `now` moments later -- the clock jump reads as
  // a multi-decade "elapsed time", incorrectly resetting the count within
  // the same boot it was just recorded in.
  bool nowValid = now >= (time_t)NTP_EPOCH_VALID_THRESHOLD;
  bool lastStrikeValid = lastStrikeEpoch >= (time_t)NTP_EPOCH_VALID_THRESHOLD;

  if (strikeHistoryValidCount > 0 && nowValid && lastStrikeValid && (now - lastStrikeEpoch) > (time_t)STRIKE_RESET_TIMEOUT_SEC)
  {
    strikeHistoryValidCount = 0;
    strikeHistoryIndex = 0;
    DEBUG_PRINTLN("Lightning strike history reset (2h+ since last strike)");
  }
}

void onConfirmedLightningStrike(int km, uint32_t energy)
{
  resetStrikeHistoryIfStale();

  strikeHistoryKm[strikeHistoryIndex] = (uint8_t)km;
  strikeHistoryEnergy[strikeHistoryIndex] = energy;
  strikeHistoryIndex = (strikeHistoryIndex + 1) % STRIKE_HISTORY_COUNT;
  if (strikeHistoryValidCount < STRIKE_HISTORY_COUNT)
  {
    strikeHistoryValidCount++;
  }

  lastStrikeEpoch = time(nullptr);
  strikeRateTimestamps[strikeRateIndex] = lastStrikeEpoch;
  strikeRateIndex = (strikeRateIndex + 1) % LIGHTNING_RATE_MAX_SAMPLES;
  if (strikeRateValidCount < LIGHTNING_RATE_MAX_SAMPLES)
  {
    strikeRateValidCount++;
  }

  lastStrikeWasStrongClose = (energy > LIGHTNING_ENERGY_HIGH_THRESHOLD) && (km < LIGHTNING_CLOSE_KM_THRESHOLD);
  if (lastStrikeWasStrongClose)
  {
    lightningAlertActive = true;
  }

  DEBUG_PRINT("Lightning strike recorded: ");
  DEBUG_PRINT(km);
  DEBUG_PRINT("km, energy=");
  DEBUG_PRINTLN(energy);
}

void handleLightningIrqWake()
{
  // Bus bring-up only, not initAs3935(): the sensor keeps its config
  // across the ESP32's deep sleep (it is never powered down), and a full
  // init writes REG0x03 (maskDisturber), whose read-modify-write clears
  // the strike interrupt this wake exists to read. The interrupt has to
  // be read before any such write consumes it.
  bool present = beginAs3935Bus();
  if (!present)
  {
    DEBUG_PRINTLN("Lightning IRQ wake: AS3935 not detected");
    return;
  }

  int km = -1;
  uint32_t energy = 0;
  readAs3935(&km, &energy);
  if (km >= 0)
  {
    onConfirmedLightningStrike(km, energy);
  }
  else
  {
    DEBUG_PRINTLN("Lightning IRQ wake: not a confirmed LIGHTNING event");
  }
}

bool shouldForceEarlyRedraw()
{
  if (lastStrikeWasStrongClose && !strongStrikeRedrawUsed)
  {
    strongStrikeRedrawUsed = true;
    return true;
  }
  return false;
}

void resetStrongStrikeRedrawLatch()
{
  strongStrikeRedrawUsed = false;
}

bool isLightningAlertActive()
{
  return lightningAlertActive;
}

void clearLightningAlert()
{
  lightningAlertActive = false;
}

void getLightningRateText(char* outText, size_t outTextSize)
{
  if (!sensorPresentCache)
  {
    snprintf(outText, outTextSize, "%s", STR_VALUE_PLACEHOLDER);
    return;
  }

  time_t now = time(nullptr);
  if (now < (time_t)NTP_EPOCH_VALID_THRESHOLD)
  {
    // Same "don't show unreliable data" rule as the rest of the panel --
    // a trailing time window is meaningless without a trustworthy clock.
    snprintf(outText, outTextSize, "%s", STR_VALUE_PLACEHOLDER);
    return;
  }

  uint8_t countInWindow = 0;
  for (uint8_t i = 0; i < strikeRateValidCount; i++)
  {
    time_t entry = strikeRateTimestamps[i];
    if (entry >= (time_t)NTP_EPOCH_VALID_THRESHOLD && (now - entry) <= (time_t)LIGHTNING_RATE_WINDOW_SEC)
    {
      countInWindow++;
    }
  }
  snprintf(outText, outTextSize, "%u", countInWindow);
}

uint8_t getStrikeHistoryCount()
{
  resetStrikeHistoryIfStale();
  return strikeHistoryValidCount;
}

void getStrikeHistoryEntry(uint8_t index, uint8_t* outKm, uint32_t* outEnergy)
{
  // index 0 = oldest of the currently-valid entries, ascending to most
  // recent. strikeHistoryIndex is the ring buffer's next-write slot; this
  // walks back strikeHistoryValidCount slots from it and steps forward by
  // `index`, wrapping through STRIKE_HISTORY_COUNT.
  uint8_t slot = (strikeHistoryIndex + STRIKE_HISTORY_COUNT - strikeHistoryValidCount + index) % STRIKE_HISTORY_COUNT;
  *outKm = strikeHistoryKm[slot];
  *outEnergy = strikeHistoryEnergy[slot];
}
