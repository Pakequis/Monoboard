/*
  Monoboard
  Main entry point - minimal setup and loop
*/

#include "config.h"
#include "debug.h"
#include "display_manager.h"
#include "time_manager.h"
#include "wifi_manager.h"
#include "dashboard_manager.h"
#include "content_manager.h"
#include "local_sensors.h"
#include "as3935_lightning.h"
#include "screen_ruler.h"
#include <time.h>
#include <esp_sleep.h>

static void goToSleep()
{
  armAs3935IrqWakeup();
  esp_sleep_enable_timer_wakeup(DEEP_SLEEP_INTERVAL_SEC * 1000000ULL);
  DEBUG_PRINTLN("Entering deep sleep...");
  esp_deep_sleep_start();
}

// RTC_DATA_ATTR so the screen's last-refresh time survives deep sleep --
// lets an AS3935 IRQ wake below decide whether a redraw is overdue,
// without needing its own always-redraw or never-redraw extreme: a
// disturber storm is already masked at the sensor (as3935_lightning.cpp),
// but a real storm with strikes landing faster than
// DEEP_SLEEP_INTERVAL_SEC could otherwise starve the redraw the same
// way. Bounding staleness to one redraw interval avoids that without
// paying the ~4.2s e-paper update cost on every single strike.
static RTC_DATA_ATTR time_t lastDrawEpoch = 0;

static bool isRedrawDue()
{
  time_t now = time(nullptr);
  return (now - lastDrawEpoch) >= (time_t)DEEP_SLEEP_INTERVAL_SEC;
}

// Redraws counted since the last anti-ghosting flush. RTC_DATA_ATTR so it
// survives deep sleep; a power cycle resets it, which also restarts the
// ~12 h schedule from that boot -- acceptable drift.
static RTC_DATA_ATTR uint32_t redrawsSinceCondition = 0;

// Flush the e-paper's accumulated ghosting before the dashboard is drawn
// over it: every DISPLAY_CONDITION_INTERVAL_SEC worth of redraws, and
// (when DISPLAY_CONDITION_ON_COLD_BOOT) once per cold boot / reset.
// Counter-based so it needs no valid wall-clock time. Call with the
// display already initialized.
static void maybeConditionPanel()
{
  const uint32_t conditionEveryN = DISPLAY_CONDITION_INTERVAL_SEC / DEEP_SLEEP_INTERVAL_SEC;
  bool scheduleDue   = redrawsSinceCondition >= conditionEveryN;
  bool coldBootFlush = DISPLAY_CONDITION_ON_COLD_BOOT &&
                       esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_UNDEFINED;

  if (scheduleDue || coldBootFlush)
  {
    DEBUG_PRINT("Conditioning e-paper (anti-ghosting flush) -- ");
    DEBUG_PRINTLN(coldBootFlush ? "cold boot" : "interval reached");
    conditionPanel(DISPLAY_CONDITION_CYCLES);
    redrawsSinceCondition = 0;
  }
  else
  {
    redrawsSinceCondition++;
  }
}

void setup()
{
  uint32_t wakeStartMs = millis();

  DEBUG_BEGIN(SERIAL_BAUD_RATE);
  DEBUG_PRINTLN();
  DEBUG_PRINTLN("========================================");
  DEBUG_PRINTLN("Monoboard - Starting up");
  DEBUG_PRINTLN("========================================");

#if SHOW_SCREEN_RULER
  // Diagnostic/templating build: draw the screen ruler and sleep for
  // good. Never returns.
  enterScreenRulerMode();
#endif

  bool irqWake = isWakeFromAs3935Irq();
  if (irqWake)
  {
    DEBUG_PRINTLN("Wake from AS3935 IRQ - checking for a strike...");
    handleLightningIrqWake();

    if (isRedrawDue())
    {
      resetCloseStrikeRedrawLatch();
      DEBUG_PRINTLN("IRQ wake, but a redraw is overdue -- drawing now instead of waiting for the timer...");
    }
    else if (shouldForceEarlyRedraw())
    {
      DEBUG_PRINTLN("IRQ wake: close strike forcing an early redraw...");
    }
    else
    {
      goToSleep();
      return;
    }
  }
  else
  {
    // Normal-cadence wake (timer) -- re-arms the forced-early-redraw
    // latch above for the next close-strike burst.
    resetCloseStrikeRedrawLatch();
  }

  // Initialize modules
  initDisplay();
  initLocalSensors();

  // Timezone must be re-applied every wake (does not survive deep sleep),
  // but WiFi only needs to come up when a sync is actually due.
  applyTimezone();

  bool syncDue = isTimeSyncDue();

  if (syncDue)
  {
    uint32_t wifiStartMs = millis();
    DEBUG_PRINTLN("Sync due - connecting WiFi...");
    if (wifiConnect())
    {
      DEBUG_PRINTLN("WiFi connected, syncing NTP...");
      syncTimeViaNTP();

      DEBUG_PRINTLN("Fetching network content...");
      fetchNetworkContent();

      DEBUG_PRINTLN("Disconnecting WiFi...");
      wifiDisconnect();
    }
    else
    {
      DEBUG_PRINTLN("WiFi connection failed - using cached data");
    }
    DEBUG_PRINT("[TIMING] WiFi phase (ms): ");
    DEBUG_PRINTLN(millis() - wifiStartMs);
  }
  else
  {
    DEBUG_PRINTLN("Sync not due yet - skipping WiFi");
  }

  DEBUG_PRINTLN("Reading local sensors...");
  readLocalSensors();

  // News carousel rotates every screen redraw, not just every sync window.
  advanceNewsCarousel();

  // Anti-ghosting flush on schedule / cold boot, before the dashboard is
  // drawn over the cleared panel.
  maybeConditionPanel();

  // Draw dashboard
  uint32_t drawStartMs = millis();
  DEBUG_PRINTLN("Drawing dashboard...");
  drawDashboard();
  lastDrawEpoch = time(nullptr);
  clearLightningAlert();
  DEBUG_PRINT("[TIMING] Draw phase (ms): ");
  DEBUG_PRINTLN(millis() - drawStartMs);

  // Power off display
  DEBUG_PRINTLN("Putting display to sleep...");
  sleepDisplay();

  DEBUG_PRINT("[TIMING] Total active time this wake (ms): ");
  DEBUG_PRINTLN(millis() - wakeStartMs);

  DEBUG_PRINTLN("Setup complete - entering deep sleep shortly...");
  DEBUG_FLUSH();

  goToSleep();
}

void loop()
{
  // Never reaches here - ESP32 is in deep sleep
}
