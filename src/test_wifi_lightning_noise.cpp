/*
  Standalone test harness (not part of the production firmware, built only
  under the wifi_lightning_noise_test PlatformIO environment): measures the
  AS3935's disturber/noise-too-high interrupt rate while alternating the
  ESP32-S3's own WiFi radio between fully off and actively transmitting, to
  check whether the WiFi radio itself raises the sensor's noise floor.

  A dedicated FreeRTOS task services the AS3935's IRQ pin independently of
  the main loop, which spends most of a WiFi-on window blocked inside HTTP
  calls -- without that separation, strikes/disturbers landing during a
  blocked HTTP call would go unread past the AS3935's ~1s window and bias
  the WiFi-on count downward for reasons unrelated to RF noise.

  Runs indefinitely, alternating fixed-length windows, printing one CSV
  summary line per window over serial. Never intended to reach a verdict on
  its own -- the collected lines are analyzed afterward, off-device.
*/

#include "config.h"
#include "debug.h"
#include "wifi_manager.h"
#include "as3935_lightning.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

namespace
{

constexpr unsigned long kWindowDurationMs = 90000UL; // 90s per WiFi state

struct WindowCounts
{
  uint32_t noiseTooHigh = 0;
  uint32_t disturber = 0;
  uint32_t lightning = 0;
  uint32_t unclassified = 0; // IRQ fired but the register read back neither of the above -- tracked as a canary, should stay 0
};

WindowCounts g_counts;
SemaphoreHandle_t g_countsMutex;
SemaphoreHandle_t g_irqSemaphore;

void IRAM_ATTR onAs3935Irq()
{
  BaseType_t woken = pdFALSE;
  xSemaphoreGiveFromISR(g_irqSemaphore, &woken);
  if (woken)
  {
    portYIELD_FROM_ISR();
  }
}

void as3935ServiceTask(void*)
{
  for (;;)
  {
    xSemaphoreTake(g_irqSemaphore, portMAX_DELAY);
    delay(2); // datasheet: the interrupt reason register needs ~2ms to settle after IRQ goes high

    int lightningKm;
    uint32_t energy;
    uint8_t raw = readAs3935Diagnostic(&lightningKm, &energy);

    xSemaphoreTake(g_countsMutex, portMAX_DELAY);
    switch (raw)
    {
      case 0x01: g_counts.noiseTooHigh++; break;
      case 0x04: g_counts.disturber++; break;
      case 0x08: g_counts.lightning++; break;
      default: g_counts.unclassified++; break;
    }
    xSemaphoreGive(g_countsMutex);

    if (raw == 0x08)
    {
      DEBUG_PRINT("STRIKE,");
      DEBUG_PRINT(millis());
      DEBUG_PRINT(",km=");
      DEBUG_PRINT(lightningKm);
      DEBUG_PRINT(",energy=");
      DEBUG_PRINTLN(energy);
    }
  }
}

// Repeated real HTTPS fetches (same endpoint/timeout the production
// firmware already uses) instead of just staying associated+idle --
// association alone doesn't reproduce the TX bursts a real sync cycle
// generates.
void generateWifiTraffic()
{
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setTimeout(WEATHER_HTTP_TIMEOUT_MS);
  if (http.begin(client, WEATHER_API_URL))
  {
    http.GET();
    http.getString(); // drain the body so the connection closes cleanly before the next attempt
    http.end();
  }
}

void printWindowSummary(unsigned long windowIndex, const char* wifiState, unsigned long actualDurationMs)
{
  xSemaphoreTake(g_countsMutex, portMAX_DELAY);
  WindowCounts snapshot = g_counts;
  g_counts = WindowCounts();
  xSemaphoreGive(g_countsMutex);

  DEBUG_PRINT("WINDOW,");
  DEBUG_PRINT(millis());
  DEBUG_PRINT(",");
  DEBUG_PRINT(windowIndex);
  DEBUG_PRINT(",");
  DEBUG_PRINT(wifiState);
  DEBUG_PRINT(",");
  DEBUG_PRINT(actualDurationMs);
  DEBUG_PRINT(",");
  DEBUG_PRINT(snapshot.noiseTooHigh);
  DEBUG_PRINT(",");
  DEBUG_PRINT(snapshot.disturber);
  DEBUG_PRINT(",");
  DEBUG_PRINT(snapshot.lightning);
  DEBUG_PRINT(",");
  DEBUG_PRINTLN(snapshot.unclassified);
}

} // namespace

void setup()
{
  DEBUG_BEGIN(SERIAL_BAUD_RATE);
  delay(200);
  DEBUG_PRINTLN();
  DEBUG_PRINTLN("========================================");
  DEBUG_PRINTLN("AS3935 vs WiFi noise test -- boot");
  DEBUG_PRINT("window_duration_ms=");
  DEBUG_PRINTLN(kWindowDurationMs);
  DEBUG_PRINTLN("========================================");
  DEBUG_PRINTLN("WINDOW,millis,window_index,wifi_state,duration_ms,noise_too_high,disturber,lightning,unclassified");

  g_countsMutex = xSemaphoreCreateMutex();
  g_irqSemaphore = xSemaphoreCreateBinary();

  bool present = initAs3935();
  DEBUG_PRINT("AS3935 present: ");
  DEBUG_PRINTLN(present ? "yes" : "NO -- test is meaningless without it");

  pinMode(PIN_AS3935_IRQ, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_AS3935_IRQ), onAs3935Irq, RISING);

  xTaskCreate(as3935ServiceTask, "as3935_svc", 4096, nullptr, configMAX_PRIORITIES - 2, nullptr);

  WiFi.mode(WIFI_OFF); // start every run from a clean, known-off state
}

void loop()
{
  static unsigned long windowIndex = 0;
  static bool wifiOn = false; // first window is OFF, establishing the baseline before any RF exposure

  unsigned long windowStart = millis();

  if (wifiOn)
  {
    wifiConnect();
    while (millis() - windowStart < kWindowDurationMs)
    {
      generateWifiTraffic();
    }
    wifiDisconnect();
  }
  else
  {
    while (millis() - windowStart < kWindowDurationMs)
    {
      delay(200);
    }
  }

  printWindowSummary(windowIndex, wifiOn ? "ON" : "OFF", millis() - windowStart);
  windowIndex++;
  wifiOn = !wifiOn;
}
