/*
  Time Manager - Implementation
  ESP32-S3 NTP client with WiFi
  Note: WiFi must be connected before calling syncTimeViaNTP()

  Uses RTC memory to persist last sync timestamp across deep sleep.
*/

#include "time_manager.h"
#include "config.h"
#include "debug.h"
#include "strings.h"
#include "calendar_math.h"

// ===== RTC memory persists across deep sleep =====
static RTC_DATA_ATTR time_t lastSyncEpoch = 0;

static bool isEpochValid(time_t epoch)
{
  return epoch >= static_cast<time_t>(NTP_EPOCH_VALID_THRESHOLD);
}

void applyTimezone()
{
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
}

bool isTimeSyncDue()
{
  time_t currentEpoch = time(nullptr);
  const time_t resyncIntervalSec = static_cast<time_t>(NTP_RESYNC_INTERVAL_MS / 1000UL);
  return (currentEpoch < static_cast<time_t>(NTP_EPOCH_VALID_THRESHOLD)) ||
         ((currentEpoch - lastSyncEpoch) >= resyncIntervalSec);
}

void syncTimeViaNTP()
{
  DEBUG_PRINT("Syncing time via NTP...");

  // Wait for NTP sync with timeout
  time_t now = time(nullptr);
  unsigned long syncStart = millis();
  while (now < NTP_EPOCH_VALID_THRESHOLD && (millis() - syncStart) < NTP_SYNC_TIMEOUT_MS)
  {
    delay(STATUS_POLL_INTERVAL_MS);
    now = time(nullptr);
    DEBUG_PRINT(".");
  }
  DEBUG_PRINTLN();

  if (now >= NTP_EPOCH_PLAUSIBLE_MIN)
  {
    DEBUG_PRINTLN("syncTimeViaNTP: NTP sync successful");
    lastSyncEpoch = now;
  }
  else if (now >= NTP_EPOCH_VALID_THRESHOLD)
  {
    // Above the "unset" threshold but before this firmware could have
    // been built -- a bad NTP response. Leave the clock untouched and
    // treat it as a failed sync so the next wake retries.
    DEBUG_PRINT("syncTimeViaNTP: implausible epoch ");
    DEBUG_PRINT((long)now);
    DEBUG_PRINTLN(" rejected - keeping previous RTC time");
  }
  else
  {
    DEBUG_PRINTLN("syncTimeViaNTP: NTP sync failed - using unsynced RTC time");
  }
}

void getTimeString(char* buffer, size_t bufferSize)
{
  time_t now = time(nullptr);
  if (!isEpochValid(now))
  {
    snprintf(buffer, bufferSize, "--:--");
    return;
  }

  const struct tm* timeinfo = localtime(&now);

  snprintf(buffer, bufferSize, "%02d:%02d",
           timeinfo->tm_hour, timeinfo->tm_min);
}

void getDateTimeString(char* buffer, size_t bufferSize)
{
  time_t now = time(nullptr);
  if (!isEpochValid(now))
  {
    snprintf(buffer, bufferSize, "--:--");
    return;
  }

  const struct tm* timeinfo = localtime(&now);

  snprintf(buffer, bufferSize, "%s, %s %d, %d %02d:%02d",
           STR_WEEKDAYS[timeinfo->tm_wday],
           STR_MONTHS[timeinfo->tm_mon],
           timeinfo->tm_mday,
           timeinfo->tm_year + 1900,
           timeinfo->tm_hour,
           timeinfo->tm_min);
}

time_t getCurrentTime()
{
  return time(nullptr);
}

void getCalendarMonthInfo(int* outFirstWeekday, int* outDaysInMonth)
{
  time_t now = time(nullptr);
  if (!isEpochValid(now))
  {
    *outFirstWeekday = 0;
    *outDaysInMonth = 0;
    return;
  }

  struct tm base = *localtime(&now);
  getCalendarMonthInfoForYearMonth(base.tm_year + 1900, base.tm_mon, outFirstWeekday, outDaysInMonth);
}
