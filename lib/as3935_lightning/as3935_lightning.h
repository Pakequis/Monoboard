#ifndef AS3935_LIGHTNING_H
#define AS3935_LIGHTNING_H

#include <Arduino.h>

// Initializes the AS3935 lightning sensor over I2C. Call once from
// setup(). Returns true if the sensor acknowledged on the bus
// (present/wired); false otherwise.
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

// Arms the AS3935's IRQ pin as an ext1 deep-sleep wakeup source. Call
// once, right before esp_deep_sleep_start() -- deep sleep resets
// peripheral configuration, so this must be re-armed every cycle.
void armAs3935IrqWakeup();

// Returns true if the current wake was caused by the AS3935's IRQ pin
// (as opposed to the deep-sleep timer, the manual-refresh button, or a
// fresh power-on).
bool isWakeFromAs3935Irq();

#endif // AS3935_LIGHTNING_H
