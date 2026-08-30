#ifndef AS3935_LIGHTNING_H
#define AS3935_LIGHTNING_H

#include <Arduino.h>

// Brings up the I2C bus for the AS3935 and checks that the sensor
// acknowledges, WITHOUT writing any configuration register. Use this on
// an AS3935 IRQ deep-sleep wake: the sensor keeps its configuration
// across the ESP32's deep sleep (it is never powered down), and any
// config write here -- maskDisturber() above all -- is a read-modify-
// write of REG0x03, the interrupt-reason register, and reading it clears
// the pending strike interrupt this wake needs to read first. Returns
// true if the sensor acknowledged on the bus.
bool beginAs3935Bus();

// Full initialization: brings up the bus (as beginAs3935Bus()) and then
// writes the sensor configuration (indoor mode, noise-rejection
// thresholds, antenna tuning, oscillator calibration, disturber
// masking). Call from a cold boot or a
// timer wake -- never from an IRQ wake, where writing REG0x03 would
// consume an unread strike interrupt. Returns true if the sensor is
// present.
bool initAs3935();

// Checks for a confirmed LIGHTNING interrupt since the last read. Returns
// true if the sensor is present and responding; false if it isn't (not
// wired). When true, lightningKmOut is set to the estimated distance to
// the most recently detected strike in kilometers and energyOut to its
// relative energy value, or lightningKmOut is set to -1 if the pending
// interrupt wasn't a confirmed LIGHTNING event (a disturber/noise event,
// or no interrupt at all) -- energyOut is left at 0 in that case.
bool readAs3935(int* lightningKmOut, uint32_t* energyOut);

// Diagnostic variant of readAs3935(): returns the AS3935's raw interrupt
// reason byte (NOISE_TO_HIGH=0x01, DISTURBER_DETECT=0x04, LIGHTNING=0x08,
// or 0x00 if the sensor isn't present or no interrupt was pending) instead
// of collapsing everything to lightning-or-not. lightningKmOut/energyOut
// behave exactly as in readAs3935().
uint8_t readAs3935Diagnostic(int* lightningKmOut, uint32_t* energyOut);

// Diagnostic: reads back and prints (via DEBUG_*) the AS3935's current
// configuration registers -- indoor/outdoor, watchdog threshold, noise
// floor, spike rejection, minimum-lightning count, disturber mask and
// tune cap. Used by the as3935_monitor test environment to characterize
// how permissive the current detection settings are; not called by the
// production firmware. No-op if the sensor isn't present.
void dumpAs3935Config();

// Arms the AS3935's IRQ pin as an ext1 deep-sleep wakeup source. Call
// once, right before esp_deep_sleep_start() -- deep sleep resets
// peripheral configuration, so this must be re-armed every cycle.
void armAs3935IrqWakeup();

// Returns true if the current wake was caused by the AS3935's IRQ pin
// (as opposed to the deep-sleep timer or a fresh power-on).
bool isWakeFromAs3935Irq();

#endif // AS3935_LIGHTNING_H
