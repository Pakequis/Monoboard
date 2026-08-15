#include "weather_client.h"
#include "config.h"
#include "debug.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

static bool parseOpenMeteoHour(const char* timestamp, time_t* outEpoch)
{
  // Open-Meteo returns local ISO-8601 timestamps as "YYYY-MM-DDTHH:MM".
  int year, month, day, hour, minute;
  if (sscanf(timestamp, "%d-%d-%dT%d:%d", &year, &month, &day, &hour, &minute) != 5)
  {
    return false;
  }

  struct tm value = {};
  value.tm_year = year - 1900;
  value.tm_mon = month - 1;
  value.tm_mday = day;
  value.tm_hour = hour;
  value.tm_min = minute;
  value.tm_isdst = -1;
  *outEpoch = mktime(&value);
  return *outEpoch != static_cast<time_t>(-1);
}

bool fetchWeatherForecast(int* outWeatherCodes, float* outTemperatures,
                          int* outIsDay, char (*outHourLabels)[4], size_t count)
{
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setTimeout(WEATHER_HTTP_TIMEOUT_MS);

  if (!http.begin(client, WEATHER_API_URL))
  {
    DEBUG_PRINTLN("fetchWeatherForecast: http.begin() failed");
    return false;
  }

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK)
  {
    DEBUG_PRINT("fetchWeatherForecast: GET failed, code=");
    DEBUG_PRINTLN(httpCode);
    http.end();
    return false;
  }

  // Open-Meteo replies with Transfer-Encoding: chunked, so go through
  // getString() (which dechunks) rather than getStream() (which doesn't).
  String payload = http.getString();
  http.end();

  StaticJsonDocument<2048> doc;
  DeserializationError err = deserializeJson(doc, payload);

  if (err)
  {
    DEBUG_PRINT("fetchWeatherForecast: JSON parse failed: ");
    DEBUG_PRINTLN(err.c_str());
    return false;
  }

  JsonArray times = doc["hourly"]["time"];
  JsonArray codes = doc["hourly"]["weathercode"];
  JsonArray temps = doc["hourly"]["temperature_2m"];
  JsonArray isDay = doc["hourly"]["is_day"];

  const size_t minimumSourceEntries = count + 1;
  if (times.isNull() || codes.isNull() || temps.isNull() || isDay.isNull() ||
      times.size() < minimumSourceEntries || codes.size() < minimumSourceEntries ||
      temps.size() < minimumSourceEntries || isDay.size() < minimumSourceEntries)
  {
    DEBUG_PRINTLN("fetchWeatherForecast: missing or short hourly arrays");
    return false;
  }

  time_t now = time(nullptr);
  struct tm nextHour = *localtime(&now);
  nextHour.tm_min = 0;
  nextHour.tm_sec = 0;
  nextHour.tm_hour++;
  nextHour.tm_isdst = -1;
  const time_t nextCompleteHour = mktime(&nextHour);

  size_t outputIndex = 0;
  for (size_t sourceIndex = 0; sourceIndex < times.size() && outputIndex < count; sourceIndex++)
  {
    const char* timestamp = times[sourceIndex];
    time_t forecastEpoch;
    if (timestamp == nullptr || !parseOpenMeteoHour(timestamp, &forecastEpoch))
    {
      DEBUG_PRINTLN("fetchWeatherForecast: invalid hourly timestamp");
      return false;
    }
    if (forecastEpoch < nextCompleteHour)
    {
      continue;
    }

    const struct tm* forecastLocalTime = localtime(&forecastEpoch);
    outWeatherCodes[outputIndex] = codes[sourceIndex].as<int>();
    outTemperatures[outputIndex] = temps[sourceIndex].as<float>();
    outIsDay[outputIndex] = isDay[sourceIndex].as<int>();
    snprintf(outHourLabels[outputIndex], 4, "%02dh", forecastLocalTime->tm_hour);
    outputIndex++;
  }

  if (outputIndex != count)
  {
    DEBUG_PRINTLN("fetchWeatherForecast: not enough future hourly entries");
    return false;
  }

  return true;
}
