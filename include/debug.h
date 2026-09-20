/*
  Debug Logging Macros
  Single point of control for all serial output: gated by APP_DEBUG_SERIAL
  in config.h. When APP_DEBUG_SERIAL is 0, these expand to nothing, so
  Serial.begin() never runs and no print call sites reach the binary.
*/

#ifndef DEBUG_H
#define DEBUG_H

#include <Arduino.h>
#include "config.h"

#if APP_DEBUG_SERIAL
  #define DEBUG_BEGIN(baud)   Serial.begin(baud)
  #define DEBUG_PRINT(...)    Serial.print(__VA_ARGS__)
  #define DEBUG_PRINTLN(...)  Serial.println(__VA_ARGS__)
  #define DEBUG_FLUSH()       Serial.flush()
  #define DEBUG_DIAG_BAUD     SERIAL_BAUD_RATE
#else
  #define DEBUG_BEGIN(baud)
  #define DEBUG_PRINT(...)
  #define DEBUG_PRINTLN(...)
  #define DEBUG_FLUSH()
  #define DEBUG_DIAG_BAUD     0
#endif

#endif // DEBUG_H
