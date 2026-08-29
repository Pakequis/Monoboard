#include "as3935_lightning.h"
#include "config.h"
#include "debug.h"
#include <Wire.h>
#include <SparkFun_AS3935.h>
#include <esp_sleep.h>

// SparkFun_AS3935's no-arg constructor is for SPI use and never sets
// _address, so I2C mode requires the explicit address constructor -- using
// defAddr (0x03), the hardware default when both ADD0/ADD1 are high.
static SparkFun_AS3935 lightning(defAddr);
static bool sensorPresent = false;

bool beginAs3935Bus()
{
  Wire.begin(PIN_AS3935_SDA, PIN_AS3935_SCL);
  sensorPresent = lightning.begin(Wire);
  return sensorPresent;
}

bool initAs3935()
{
  if (!beginAs3935Bus())
  {
    return false;
  }

  lightning.setIndoorOutdoor(INDOOR);
  // AS3935_TUNE_CAP (config.h) was measured once via an IRQ-pin pulse-count
  // sweep across all 16 steps -- 0pF was already the closest to the
  // 500kHz antenna target (496480Hz, 0.7% off), so no extra capacitance
  // is added; this call just makes that verified value explicit rather
  // than relying on it silently matching the chip's power-on default.
  lightning.tuneCap(AS3935_TUNE_CAP);
  lightning.calibrateOsc();
  // Without this, every disturber event (common indoors -- WiFi,
  // switching power supplies, etc.) raises the IRQ pin exactly like a
  // real strike would, and since that pin is the ext1 deep-sleep wake
  // source, a disturber storm re-wakes the board faster than the
  // screen's own redraw timer, starving it forever. Confirmed via
  // serial: raw interrupt register read 0x4 (DISTURBER_DETECT) on every
  // wake in a tight loop, never reaching the timer-driven redraw path.
  //
  // maskDisturber() is a read-modify-write of REG0x03 -- the same
  // register that reports the interrupt reason -- and reading REG0x03
  // clears its pending interrupt bits. That is why the AS3935 IRQ wake
  // path uses beginAs3935Bus() + readAs3935() rather than this function:
  // it has to read the strike interrupt before any such write consumes
  // it. Safe to write here because initAs3935() only runs on a cold boot
  // or a timer wake, when no unread strike interrupt is pending.
  lightning.maskDisturber(true);
  return true;
}

uint8_t readAs3935Diagnostic(int* lightningKmOut, uint32_t* energyOut)
{
  if (!sensorPresent)
  {
    return 0x00;
  }

  uint8_t interruptSource = lightning.readInterruptReg();
  if (interruptSource == LIGHTNING)
  {
    *lightningKmOut = lightning.distanceToStorm();
    *energyOut = lightning.lightningEnergy();
  }
  else
  {
    *lightningKmOut = -1;
    *energyOut = 0;
  }
  return interruptSource;
}

bool readAs3935(int* lightningKmOut, uint32_t* energyOut)
{
  if (!sensorPresent)
  {
    return false;
  }

  readAs3935Diagnostic(lightningKmOut, energyOut);
  return true;
}

void armAs3935IrqWakeup()
{
  // Logged only on failure (not every wake) -- this is the only runtime
  // signal that GPIO 6 was actually accepted as an ext1 wake source (the
  // ESP-IDF call has no compile-time pin validation; an invalid pin here
  // would silently fail to arm the source rather than fail the build).
  esp_err_t err = esp_sleep_enable_ext1_wakeup(1ULL << PIN_AS3935_IRQ, ESP_EXT1_WAKEUP_ANY_HIGH);
  if (err != ESP_OK)
  {
    DEBUG_PRINT("armAs3935IrqWakeup failed, esp_err_t=");
    DEBUG_PRINTLN(err);
  }
}

bool isWakeFromAs3935Irq()
{
  return esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT1;
}
