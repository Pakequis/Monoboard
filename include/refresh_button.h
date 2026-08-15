#ifndef REFRESH_BUTTON_H
#define REFRESH_BUTTON_H

// Returns true if the current wake was caused by the manual-refresh
// button (as opposed to the deep-sleep timer or a fresh power-on).
bool isWakeFromButton();

// Arms the button's GPIO as an ext0 deep-sleep wakeup source. Call once,
// right before esp_deep_sleep_start() — deep sleep resets peripheral
// configuration, so this must be re-armed every cycle.
void armRefreshButtonWakeup();

#endif // REFRESH_BUTTON_H
