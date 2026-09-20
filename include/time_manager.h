/*
  Time Manager - NTP sync and time formatting
  ESP32-S3 with WiFi
*/

#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>

// Re-apply timezone/SNTP config. Cheap, local, no WiFi required — call every
// wake, since this state does not survive deep sleep (only the RTC-backed
// epoch does).
void applyTimezone();

// Check whether an NTP resync is due (NTP_RESYNC_INTERVAL_MS since last
// success). No WiFi required to call.
bool isTimeSyncDue();

// Perform the actual blocking NTP sync. WiFi must already be connected.
void syncTimeViaNTP();

// Format current time as "HH:MM" into buffer (must be at least 6 bytes)
void getTimeString(char* buffer, size_t bufferSize);

// Format combined date and time as "Weekday, Month DD, YYYY HH:MM" into buffer (must be at least 128 bytes)
void getDateTimeString(char* buffer, size_t bufferSize);

// Get current epoch time
time_t getCurrentTime();

// Fills outFirstWeekday (0=Sunday..6=Saturday, matching struct tm's
// tm_wday) and outDaysInMonth with the current month's calendar shape,
// computed via mktime()'s field normalization rather than a hand-rolled
// leap-year table. If the epoch isn't valid yet (no NTP sync since boot),
// fills both with 0 -- the caller should skip drawing the day grid in that
// case, same placeholder rule as the header clock/date.
void getCalendarMonthInfo(int* outFirstWeekday, int* outDaysInMonth);

#endif // TIME_MANAGER_H