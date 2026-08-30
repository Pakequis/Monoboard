/*
  Display Manager for E-Paper
  Handles all display operations (text, lines, shapes, screen updates)
*/

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>

// Must be defined before the GxEPD2 headers are first included in any
// translation unit, or the GxEPD2_BW/GxEPD2_3C class layout can disagree
// across .cpp files. Every file that needs GxEPD2 goes through this header.
#define ENABLE_GxEPD2_GFX 0
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>

// Display type: Waveshare 7.5 inch ePaper, 640x384
extern GxEPD2_BW<GxEPD2_750, GxEPD2_750::HEIGHT> display;

// Initialize the e-paper display
void initDisplay();

// Clear the screen (fill with white)
void clearScreen();

// Write text at specified position with given font
void writeText(int16_t x, int16_t y, const char* text, const GFXfont* font = nullptr, uint16_t color = GxEPD_BLACK);

// Write text centered on the screen
void writeTextCentered(const char* text, const GFXfont* font = nullptr, uint16_t color = GxEPD_BLACK);

// Write text horizontally centered within [boxX0, boxX0+boxWidth), baseline
// at y. Unlike centering by ink width alone, this also cancels out the
// glyph's own left bearing (tbx) -- needed for fonts like DSEG7 where digits
// carry a large, very unequal left bearing baked in to mimic real
// seven-segment displays (e.g. "1" sits far to the right of its own cell).
void writeTextCenteredInBox(int16_t boxX0, int16_t boxWidth, int16_t y, const char* text,
                            const GFXfont* font = nullptr, uint16_t color = GxEPD_BLACK);

// Get pixel width of a text string with optional font
int16_t getTextWidth(const char* text, const GFXfont* font = nullptr);

// Draw a line from (x1, y1) to (x2, y2)
void drawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color = GxEPD_BLACK);

// Draw a rectangle outline
void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color = GxEPD_BLACK);

// Draw a filled rectangle
void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color = GxEPD_BLACK);

// Draw a circle outline
void drawCircle(int16_t x, int16_t y, int16_t r, uint16_t color = GxEPD_BLACK);

// Draw a filled circle
void fillCircle(int16_t x, int16_t y, int16_t r, uint16_t color = GxEPD_BLACK);

// Draw a dashed circle outline: alternating drawn/skipped short arcs
// (each rendered as a straight chord between two nearby points on the
// circle, so dash length stays consistent regardless of radius),
// dashLength px long, separated by gapLength px gaps. Adafruit_GFX has no
// built-in dashed-line primitive.
void drawDashedCircle(int16_t x, int16_t y, int16_t r, int16_t dashLength, int16_t gapLength, uint16_t color = GxEPD_BLACK);

// Draw a circle outline with a single gap, centered at gapCenterAngleRad
// (radians, standard atan2 convention) and gapLengthPx wide (converted to
// an angle from the radius, so the gap covers the same arc-length -- and
// stays clear of the same amount of on-screen content -- at any radius).
// Same short-chord approximation as drawDashedCircle().
void drawArc(int16_t x, int16_t y, int16_t r, float gapCenterAngleRad, int16_t gapLengthPx, uint16_t color = GxEPD_BLACK);

// Draw a stylized lightning bolt (2 overlapping filled triangles) inside
// the bounding box (x, y)-(x+w, y+h). Used as the close-strike alert
// icon on the lightning box.
void drawLightningBolt(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color = GxEPD_BLACK);

// Callback type for paged refresh drawing
typedef void (*DrawFunction)();

// Update the display (refresh screen) using a paged draw callback
void updateScreen(DrawFunction drawFunc);

// Flush the panel with `cycles` black<->white full-refresh pairs to clear
// accumulated ghosting from a long run of near-identical images, leaving
// it white. Blocks ~9 s per cycle (two ~4.5 s full refreshes). The caller
// is expected to redraw content afterwards. See DISPLAY_CONDITION_* in
// config.h.
void conditionPanel(uint8_t cycles);

// Put display to sleep (power off)
void sleepDisplay();

#endif // DISPLAY_MANAGER_H