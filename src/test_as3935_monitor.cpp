/*
  Standalone AS3935 event monitor (not part of the production firmware,
  built only under the as3935_monitor PlatformIO environment).

  Purpose: answer "are the strikes the dashboard is counting real?". The
  board stays awake and services the AS3935's IRQ pin continuously,
  printing one line per interrupt with its classification straight from
  the chip -- NOISE_TO_HIGH (0x01), DISTURBER (0x04) or LIGHTNING (0x08)
  -- plus the reported distance and energy for lightning events. A
  genuine strike near 1 km arrives with thunder within a few seconds; a
  run full of LIGHTNING lines under a clear sky with no thunder, all at
  ~1 km and low energy, is the sensor misclassifying local electrical
  noise.

  The chip config is dumped once at boot (dumpAs3935Config) so the
  readback values -- watchdog threshold, noise floor, spike rejection,
  minimum-lightning count -- are on the same log as the events they
  produced.

  Runs indefinitely; prints a heartbeat with running counts every
  MONITOR_HEARTBEAT_MS so a quiet stretch is distinguishable from a hung
  board. Analyzed by eye, off-device.
*/

#include "config.h"
#include "debug.h"
#include "as3935_lightning.h"
#include <Arduino.h>

namespace
{

constexpr unsigned long MONITOR_HEARTBEAT_MS = 60000UL;

volatile bool g_irqFlag = false;

uint32_t g_noiseTooHigh = 0;
uint32_t g_disturber = 0;
uint32_t g_lightning = 0;
uint32_t g_unclassified = 0; // IRQ fired but the register read back as none of the above

void IRAM_ATTR onAs3935Irq()
{
  g_irqFlag = true;
}

void serviceEvent()
{
  delay(2); // datasheet: the interrupt reason register needs ~2ms to settle after IRQ goes high

  int lightningKm = -1;
  uint32_t energy = 0;
  uint8_t raw = readAs3935Diagnostic(&lightningKm, &energy);

  const char* label = "UNCLASSIFIED";
  switch (raw)
  {
    case 0x01: g_noiseTooHigh++;  label = "NOISE_TOO_HIGH"; break;
    case 0x04: g_disturber++;     label = "DISTURBER";      break;
    case 0x08: g_lightning++;     label = "LIGHTNING";      break;
    default:   g_unclassified++;                            break;
  }

  DEBUG_PRINT("EVENT,");
  DEBUG_PRINT(millis());
  DEBUG_PRINT(",");
  DEBUG_PRINT(label);
  DEBUG_PRINT(",raw=0x");
  DEBUG_PRINT(raw, HEX);
  DEBUG_PRINT(",km=");
  DEBUG_PRINT(lightningKm);
  DEBUG_PRINT(",energy=");
  DEBUG_PRINTLN(energy);
}

void printHeartbeat()
{
  DEBUG_PRINT("HEARTBEAT,");
  DEBUG_PRINT(millis());
  DEBUG_PRINT(",noise=");
  DEBUG_PRINT(g_noiseTooHigh);
  DEBUG_PRINT(",disturber=");
  DEBUG_PRINT(g_disturber);
  DEBUG_PRINT(",lightning=");
  DEBUG_PRINT(g_lightning);
  DEBUG_PRINT(",unclassified=");
  DEBUG_PRINTLN(g_unclassified);
}

} // namespace

void setup()
{
  DEBUG_BEGIN(SERIAL_BAUD_RATE);
  delay(200);
  DEBUG_PRINTLN();
  DEBUG_PRINTLN("========================================");
  DEBUG_PRINTLN("AS3935 event monitor -- boot");
  DEBUG_PRINTLN("========================================");

  bool present = false;
  for (int attempt = 1; attempt <= 10 && !present; attempt++)
  {
    present = initAs3935();
    DEBUG_PRINT("AS3935 begin attempt ");
    DEBUG_PRINT(attempt);
    DEBUG_PRINTLN(present ? ": ok" : ": no ACK on I2C, retrying in 500ms");
    if (!present)
    {
      delay(500);
    }
  }
  DEBUG_PRINT("AS3935 present: ");
  DEBUG_PRINTLN(present ? "yes" : "NO -- check wiring / power-cycle the board");
  dumpAs3935Config();
  DEBUG_PRINTLN("----------------------------------------");
  DEBUG_PRINTLN("EVENT,millis,class,raw,km,energy");

  pinMode(PIN_AS3935_IRQ, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_AS3935_IRQ), onAs3935Irq, RISING);

  // Clear any interrupt latched during init so the first logged event is
  // a fresh one.
  int km;
  uint32_t energy;
  readAs3935Diagnostic(&km, &energy);
}

void loop()
{
  static unsigned long lastHeartbeat = 0;

  if (g_irqFlag)
  {
    g_irqFlag = false;
    serviceEvent();
  }

  if (millis() - lastHeartbeat >= MONITOR_HEARTBEAT_MS)
  {
    lastHeartbeat = millis();
    printHeartbeat();
  }

  delay(5);
}
