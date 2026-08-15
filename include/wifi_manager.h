/*
  WiFi Manager
  Centralizes WiFi connection/disconnection for all modules
*/

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>

// Connect to WiFi using configured credentials with timeout
// Returns true if connected
bool wifiConnect();

// Disconnect WiFi to save power
void wifiDisconnect();

// Check current WiFi connection status
bool isWifiConnected();

#endif // WIFI_MANAGER_H