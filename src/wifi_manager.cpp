/*
  WiFi Manager - Implementation
  Centralized WiFi connection control
*/

#include "wifi_manager.h"
#include "config.h"
#include "debug.h"
#include <WiFi.h>
#include <string.h>

// Persists across deep sleep so the footer still reflects the last
// connection attempt's result on wake cycles that skip WiFi entirely.
static RTC_DATA_ATTR bool lastConnected = false;

// Last known-good BSSID/channel, so wifiConnect() can skip the channel
// scan on the common case (same AP still up on the same channel) --
// cuts typical association time from a couple of seconds to a few
// hundred ms. Falls back to a full scan-and-connect if the fast path
// doesn't work (AP changed channel, went down, etc.).
static RTC_DATA_ATTR uint8_t lastBSSID[6] = {0, 0, 0, 0, 0, 0};
static RTC_DATA_ATTR int32_t lastChannel = 0;
static RTC_DATA_ATTR bool haveLastBSSID = false;

static bool waitForConnection(unsigned long timeoutMs)
{
  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED &&
         (millis() - startAttempt) < timeoutMs)
  {
    delay(STATUS_POLL_INTERVAL_MS);
  }
  return WiFi.status() == WL_CONNECTED;
}

bool wifiConnect()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    WiFi.mode(WIFI_STA);

    if (haveLastBSSID)
    {
      DEBUG_PRINTLN("wifiConnect: trying fast reconnect (known BSSID/channel)...");
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD, lastChannel, lastBSSID, true);

      if (!waitForConnection(WIFI_FAST_RECONNECT_TIMEOUT_MS))
      {
        DEBUG_PRINTLN("wifiConnect: fast reconnect failed - falling back to full scan...");
        WiFi.disconnect();
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        waitForConnection(WIFI_TIMEOUT_MS - WIFI_FAST_RECONNECT_TIMEOUT_MS);
      }
    }
    else
    {
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      waitForConnection(WIFI_TIMEOUT_MS);
    }
  }

  lastConnected = (WiFi.status() == WL_CONNECTED);
  if (lastConnected)
  {
    memcpy(lastBSSID, WiFi.BSSID(), sizeof(lastBSSID));
    lastChannel = WiFi.channel();
    haveLastBSSID = true;
  }
  return lastConnected;
}

void wifiDisconnect()
{
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

bool isWifiConnected()
{
  return lastConnected;
}