#ifndef WEATHER_CLIENT_H
#define WEATHER_CLIENT_H

#include <Arduino.h>

// Fetches `count` forecast intervals for the configured location. The interval
// currently in progress is excluded, so entry 0 is the next complete local
// hour. outHourLabels receives the matching "HHh" labels; each element needs
// four bytes including the terminator. WiFi must already be connected.
bool fetchWeatherForecast(int* outWeatherCodes, float* outTemperatures,
                          int* outIsDay, char (*outHourLabels)[4], size_t count);

#endif // WEATHER_CLIENT_H
