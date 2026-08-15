#include "refresh_button.h"
#include "config.h"
#include <Arduino.h>
#include <esp_sleep.h>
#include "driver/gpio.h"

bool isWakeFromButton()
{
  return esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0;
}

void armRefreshButtonWakeup()
{
  pinMode(PIN_REFRESH_BUTTON, INPUT_PULLUP);
  esp_sleep_enable_ext0_wakeup(static_cast<gpio_num_t>(PIN_REFRESH_BUTTON), 0);
  // gpio_pullup_en/gpio_pulldown_dis (not the rtc_gpio_* equivalents): the
  // rtc_gpio_* pull calls corrupted the RTC-backed system clock across deep
  // sleep on this board, making isTimeSyncDue() see an invalid time and
  // forcing an NTP resync on every wake instead of hourly. The fault was
  // isolated by comparing wake behavior with the GPIO pull configured both
  // ways (rtc_gpio_* vs gpio_*), confirming the corruption came specifically
  // from the rtc_gpio_* calls.
  gpio_pullup_en(static_cast<gpio_num_t>(PIN_REFRESH_BUTTON));
  gpio_pulldown_dis(static_cast<gpio_num_t>(PIN_REFRESH_BUTTON));
}
