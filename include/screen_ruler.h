#ifndef SCREEN_RULER_H
#define SCREEN_RULER_H

// Screen-measurement reference mode. Draws centimetre rulers along all
// four edges of the panel plus a centred 10 cm calibration bar, latches
// the image on the e-paper, and deep-sleeps forever (no timer, no IRQ
// wake armed) so the board can be powered off and the frozen screen used
// as a physical template -- e.g. for cutting an enclosure.
//
// Compiled into the production firmware but only reached when
// SHOW_SCREEN_RULER (config.h) is non-zero; main.cpp calls this before
// any normal-boot work. Never returns.
void enterScreenRulerMode();

#endif // SCREEN_RULER_H
