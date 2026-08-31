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

  Flash logging: every HEARTBEAT line and every non-NOISE event
  (LIGHTNING / DISTURBER / UNCLASSIFIED) is also appended to a file on
  the on-chip LittleFS partition, so the board can be left running on a
  bare USB charger with no host attached and the record read back later.
  Individual NOISE_TOO_HIGH lines are kept on serial only -- the 60 s
  heartbeat already carries their running total, so persisting each one
  would just wear the flash. On boot the existing file is streamed to
  serial (wrapped in FLASHLOG markers) before new lines are appended, and
  the file is never truncated, so it accumulates across power cycles. A
  boot counter in NVS labels each power-on segment since millis() resets
  with it.
*/

#include "config.h"
#include "debug.h"
#include "as3935_lightning.h"
#include <Arduino.h>
#include <LittleFS.h>
#include <Preferences.h>

namespace
{

constexpr unsigned long MONITOR_HEARTBEAT_MS = 60000UL;

// Path of the persistent event log on the LittleFS ("spiffs" subtype)
// partition. Stop appending once free space drops below the reserve so a
// full filesystem degrades to "serial only" instead of thrashing failed
// writes.
constexpr const char* FLASH_LOG_PATH = "/as3935.log";
constexpr size_t FLASH_LOG_MIN_FREE = 24u * 1024u;

bool g_flashReady = false;

volatile bool g_irqFlag = false;

uint32_t g_noiseTooHigh = 0;
uint32_t g_disturber = 0;
uint32_t g_lightning = 0;
uint32_t g_unclassified = 0; // IRQ fired but the register read back as none of the above

void IRAM_ATTR onAs3935Irq()
{
  g_irqFlag = true;
}

// Append one line to the flash log, opening and closing the file each
// call -- the event rate is low (a heartbeat a minute plus real strikes)
// and a file left open across a yanked-power event is the main way
// LittleFS corrupts.
void appendFlashLog(const char* line)
{
  if (!g_flashReady)
  {
    return;
  }
  if (LittleFS.totalBytes() - LittleFS.usedBytes() < FLASH_LOG_MIN_FREE)
  {
    return;
  }
  File f = LittleFS.open(FLASH_LOG_PATH, "a");
  if (!f)
  {
    return;
  }
  f.println(line);
  f.close();
}

#if APP_DEBUG_SERIAL
// Stream whatever the flash log already holds (possibly several past
// power-on segments) to serial, wrapped in markers a capture script can
// bracket on.
void dumpFlashLog()
{
  DEBUG_PRINTLN("----- FLASHLOG DUMP START -----");
  File f = LittleFS.open(FLASH_LOG_PATH, "r");
  if (f)
  {
    while (f.available())
    {
      Serial.write(f.read());
    }
    f.close();
  }
  else
  {
    DEBUG_PRINTLN("(no flash log yet)");
  }
  DEBUG_PRINTLN("----- FLASHLOG DUMP END -----");
}
#else
void dumpFlashLog() {}
#endif

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

  char buf[96];
  snprintf(buf, sizeof(buf), "EVENT,%lu,%s,raw=0x%X,km=%d,energy=%lu",
           millis(), label, raw, lightningKm, (unsigned long)energy);
  DEBUG_PRINTLN(buf);

  // NOISE_TOO_HIGH is the running total in every heartbeat already; only
  // the events that matter go to flash, in full.
  if (raw != 0x01)
  {
    appendFlashLog(buf);
  }
}

void printHeartbeat()
{
  char buf[96];
  snprintf(buf, sizeof(buf),
           "HEARTBEAT,%lu,noise=%lu,disturber=%lu,lightning=%lu,unclassified=%lu",
           millis(), (unsigned long)g_noiseTooHigh, (unsigned long)g_disturber,
           (unsigned long)g_lightning, (unsigned long)g_unclassified);
  DEBUG_PRINTLN(buf);
  appendFlashLog(buf);
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

  g_flashReady = LittleFS.begin(true); // format on first use / corruption
  DEBUG_PRINT("LittleFS mount: ");
  DEBUG_PRINTLN(g_flashReady ? "ok" : "FAILED -- serial-only this run");

  // Per-power-on segment marker: millis() restarts from zero every boot,
  // so without this the flash log's segments can't be told apart.
  uint32_t bootCount = 0;
  {
    Preferences prefs;
    if (prefs.begin("as3935mon", false))
    {
      bootCount = prefs.getUInt("boot", 0) + 1;
      prefs.putUInt("boot", bootCount);
      prefs.end();
    }
  }

  dumpFlashLog();

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

  // Flash-log segment header: the compiled sensitivity constants (the
  // live register readback goes to serial via dumpAs3935Config above).
  char buf[160];
  snprintf(buf, sizeof(buf),
           "==== BOOT %lu, present=%d, wdth=%d nf=%d srej=%d minstk=%d mask=1 cap=%d ====",
           (unsigned long)bootCount, present ? 1 : 0,
           AS3935_WATCHDOG_THRESHOLD, AS3935_NOISE_LEVEL, AS3935_SPIKE_REJECTION,
           AS3935_MIN_STRIKES, AS3935_TUNE_CAP);
  appendFlashLog(buf);

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
