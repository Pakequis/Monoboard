/*
  Screen-measurement reference mode -- see screen_ruler.h.

  Panel: Waveshare 7.5" GxEPD2_750, 640 x 384 px. Datasheet active area
  is 163.2 x 97.92 mm (0.255 mm dot pitch, isotropic), but a caliper on
  an early build's calibration bar read 100.28 mm for a nominal 100 mm,
  so SCALE_CORRECTION below trims every dimension to this unit's real
  pitch (confirmed 100.0 mm after the trim). The scale is isotropic, so
  the one horizontal calibration bar also validates vertical scaling.

  On-screen text is English and ASCII only, because the Adafruit GFX
  fonts bundled here carry no Latin-1 glyphs; the one accented character
  (the title's "Pakequis") is drawn by hand.
*/

#include "screen_ruler.h"
#include "config.h"
#include "debug.h"
#include "display_manager.h"
#include <Arduino.h>
#include <esp_sleep.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <Fonts/FreeMonoBold9pt7b.h>

namespace
{

constexpr int PANEL_W_PX = 640;
constexpr int PANEL_H_PX = 384;

// Datasheet nominal: 640 px / 163.2 mm == 384 px / 97.92 mm == 0.255 mm
// dot pitch, isotropic.
constexpr float PX_PER_MM_NOMINAL = PANEL_W_PX / 163.2f;

// Empirical trim for this physical unit: an early build's 10 cm
// calibration bar measured 100.28 mm with a caliper (tick-centre to
// tick-centre), so the real pitch runs ~0.28% longer than nominal.
// Every dimension below is scaled by this so the printed rulers match a
// real ruler laid on THIS panel. If the panel is swapped, re-measure the
// bar and set this to 100.0 / <new measured mm>.
constexpr float SCALE_CORRECTION = 100.0f / 100.28f;

constexpr float PX_PER_MM = PX_PER_MM_NOMINAL * SCALE_CORRECTION;

int mmToPx(float mm)
{
  return (int)lroundf(mm * PX_PER_MM);
}

void printCentered(int baselineY, const char* s)
{
  int16_t bx, by;
  uint16_t bw, bh;
  display.getTextBounds(s, 0, 0, &bx, &by, &bw, &bh);
  display.setCursor((PANEL_W_PX - (int)bw) / 2 - bx, baselineY);
  display.print(s);
}

void drawEdgeRulers()
{
  const int MM_TICK      = 4;
  const int HALF_CM_TICK = 8;
  const int CM_TICK      = 15;

  display.setFont();          // built-in 6x8 font
  display.setTextSize(1);
  display.setTextColor(GxEPD_BLACK);

  display.drawRect(0, 0, PANEL_W_PX, PANEL_H_PX, GxEPD_BLACK);

  // Top and bottom edges share the same x grid, zero at the left corner.
  for (int mm = 0; mm * PX_PER_MM <= PANEL_W_PX - 1; ++mm)
  {
    int x   = mmToPx(mm);
    int len = (mm % 10 == 0) ? CM_TICK : (mm % 5 == 0 ? HALF_CM_TICK : MM_TICK);
    display.drawFastVLine(x, 1, len, GxEPD_BLACK);
    display.drawFastVLine(x, PANEL_H_PX - 1 - len, len, GxEPD_BLACK);

    if (mm % 10 == 0)
    {
      char b[4];
      snprintf(b, sizeof(b), "%d", mm / 10);
      int tw = (int)strlen(b) * 6;
      int lx = x + 2;
      if (lx + tw > PANEL_W_PX - 2)
      {
        lx = x - tw - 1;
      }
      display.setCursor(lx, CM_TICK + 3);
      display.print(b);
      display.setCursor(lx, PANEL_H_PX - 1 - CM_TICK - 10);
      display.print(b);
    }
  }

  // Left and right edges share the same y grid, zero at the top corner.
  for (int mm = 0; mm * PX_PER_MM <= PANEL_H_PX - 1; ++mm)
  {
    int y   = mmToPx(mm);
    int len = (mm % 10 == 0) ? CM_TICK : (mm % 5 == 0 ? HALF_CM_TICK : MM_TICK);
    display.drawFastHLine(1, y, len, GxEPD_BLACK);
    display.drawFastHLine(PANEL_W_PX - 1 - len, y, len, GxEPD_BLACK);

    // mm 0 skipped: the top ruler's "0" already labels that corner.
    if (mm % 10 == 0 && mm > 0)
    {
      char b[4];
      snprintf(b, sizeof(b), "%d", mm / 10);
      int tw = (int)strlen(b) * 6;
      int ly = y + 2;
      if (ly + 8 > PANEL_H_PX - 2)
      {
        ly = y - 9;
      }
      display.setCursor(CM_TICK + 3, ly);
      display.print(b);
      display.setCursor(PANEL_W_PX - 1 - CM_TICK - 3 - tw, ly);
      display.print(b);
    }
  }
}

void drawTitle()
{
  const int baseY = 165;
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextSize(1);
  display.setTextColor(GxEPD_BLACK);

  // Centre on the accent-free spelling so the hand-drawn mark doesn't
  // shift the layout.
  const char* full = "Pakequis - Screen Measurement Test";
  int16_t bx, by;
  uint16_t bw, bh;
  display.getTextBounds(full, 0, 0, &bx, &by, &bw, &bh);
  int x = (PANEL_W_PX - (int)bw) / 2 - bx;

  display.setCursor(x, baseY);
  display.print("Pak");
  int ex = display.getCursorX();
  display.print("e");
  // Acute accent over the 'e': a short thick diagonal above the glyph.
  display.drawLine(ex + 2, baseY - 11, ex + 7, baseY - 15, GxEPD_BLACK);
  display.drawLine(ex + 2, baseY - 12, ex + 7, baseY - 16, GxEPD_BLACK);
  display.print("quis - Screen Measurement Test");
}

void drawCenterInfo()
{
  display.setFont();
  display.setTextSize(2);
  display.setTextColor(GxEPD_BLACK);
  printCentered(192, "Active area: 163.7 x 98.2 mm (calibrated)");
  printCentered(214, "640 x 384 px   |   39.11 px/cm");

  // 10 cm calibration bar, ticks pointing up.
  const int calY = 252;
  const int x0   = (PANEL_W_PX - mmToPx(100)) / 2;
  const int x1   = x0 + mmToPx(100);
  display.drawFastHLine(x0, calY, x1 - x0 + 1, GxEPD_BLACK);
  for (int mm = 0; mm <= 100; ++mm)
  {
    int x   = x0 + mmToPx(mm);
    int len = (mm % 10 == 0) ? 10 : (mm % 5 == 0 ? 6 : 3);
    display.drawFastVLine(x, calY - len, len, GxEPD_BLACK);
  }

  display.setTextSize(1);
  display.setCursor(x0 - 2, calY + 5);
  display.print("0");
  display.setCursor(x1 - 28, calY + 5);
  display.print("10 cm");
  printCentered(calY + 22, "this bar should measure 100 mm on a real ruler");
}

void drawRulerScreen()
{
  drawEdgeRulers();
  drawTitle();
  drawCenterInfo();
}

} // namespace

void enterScreenRulerMode()
{
  DEBUG_PRINTLN("SHOW_SCREEN_RULER set -- drawing measurement reference");
  initDisplay();
  updateScreen(drawRulerScreen);
  sleepDisplay();
  DEBUG_PRINTLN("Screen ruler latched -- deep sleep with no wake source");
  DEBUG_FLUSH();

  // No timer, no ext1 IRQ armed: the board stays asleep until a manual
  // reset or power cycle. Reflash with SHOW_SCREEN_RULER=0 to return to
  // the normal firmware.
  esp_deep_sleep_start();
}
