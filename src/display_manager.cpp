/*
  Display Manager for E-Paper - Implementation
  ESP32-S3 DevKitC-1
*/

#include "display_manager.h"
#include "config.h"
#include "debug.h"
#include <Wire.h>
#include <math.h>
#include <Fonts/FreeMonoBold9pt7b.h>

/*
  ESP32-S3 default SPI pins:
    MOSI = GPIO 11, SCK = GPIO 12, SS = GPIO 10

  Waveshare 7.5 inch ePaper display pin mapping
*/

static const uint8_t PIN_BUSY = 4;
static const uint8_t PIN_RST  = 16;
static const uint8_t PIN_DC   = 17;
static const uint8_t PIN_CS   = 10;
static const uint8_t PIN_SCK  = 12;
static const uint8_t PIN_MOSI = 11;

/* Configure Waveshare 7.5 inch ePaper display pins */
GxEPD2_BW<GxEPD2_750, GxEPD2_750::HEIGHT> display(GxEPD2_750(PIN_CS, PIN_DC, PIN_RST, PIN_BUSY));

void initDisplay()
{
  DEBUG_PRINTLN("initDisplay: resetting display...");
  
  // Hardware reset: pulse RST pin low
  pinMode(PIN_RST, OUTPUT);
  digitalWrite(PIN_RST, LOW);
  delay(DISPLAY_RESET_PULSE_MS);
  digitalWrite(PIN_RST, HIGH);
  delay(DISPLAY_RESET_PULSE_MS);

  DEBUG_PRINTLN("initDisplay: calling display.init()...");
  display.init(DEBUG_DIAG_BAUD, true, DISPLAY_RESET_PULSE_MS, false);
  // Set rotation: 0 = landscape (native, 640x384), 1 = portrait
  display.setRotation(0);
  DEBUG_PRINTLN("initDisplay: done");
}

void clearScreen()
{
  display.fillScreen(GxEPD_WHITE);
}

void writeText(int16_t x, int16_t y, const char* text, const GFXfont* font, uint16_t color)
{
  if (font != nullptr)
  {
    display.setFont(font);
  }
  display.setTextColor(color);
  display.setCursor(x, y);
  display.print(text);
}

void writeTextCentered(const char* text, const GFXfont* font, uint16_t color)
{
  if (font != nullptr)
  {
    display.setFont(font);
  }
  display.setTextColor(color);

  int16_t tbx, tby;
  uint16_t tbw, tbh;
  display.getTextBounds(text, 0, 0, &tbx, &tby, &tbw, &tbh);

  // Signed arithmetic avoids wraparound if the text is wider/taller than
  // the display; clamp negative results to 0 instead of wrapping to a huge
  // uint16_t value.
  int32_t rawX = ((static_cast<int32_t>(display.width()) - static_cast<int32_t>(tbw)) / 2) - tbx;
  int32_t rawY = ((static_cast<int32_t>(display.height()) - static_cast<int32_t>(tbh)) / 2) - tby;

  int16_t x = (rawX > 0) ? static_cast<int16_t>(rawX) : static_cast<int16_t>(0);
  int16_t y = (rawY > 0) ? static_cast<int16_t>(rawY) : static_cast<int16_t>(0);

  display.setCursor(x, y);
  display.print(text);
}

void writeTextCenteredInBox(int16_t boxX0, int16_t boxWidth, int16_t y, const char* text,
                            const GFXfont* font, uint16_t color)
{
  if (font != nullptr)
  {
    display.setFont(font);
  }
  display.setTextColor(color);

  int16_t tbx, tby;
  uint16_t tbw, tbh;
  display.getTextBounds(text, 0, 0, &tbx, &tby, &tbw, &tbh);

  int16_t x = boxX0 + (boxWidth - static_cast<int16_t>(tbw)) / 2 - tbx;
  display.setCursor(x, y);
  display.print(text);
}

int16_t getTextWidth(const char* text, const GFXfont* font)
{
  if (font != nullptr)
  {
    display.setFont(font);
  }
  int16_t tbx, tby;
  uint16_t tbw, tbh;
  display.getTextBounds(text, 0, 0, &tbx, &tby, &tbw, &tbh);
  return tbw;
}

void drawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{
  display.drawLine(x1, y1, x2, y2, color);
}

void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
  display.drawRect(x, y, w, h, color);
}

void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
  display.fillRect(x, y, w, h, color);
}

void drawCircle(int16_t x, int16_t y, int16_t r, uint16_t color)
{
  display.drawCircle(x, y, r, color);
}

void fillCircle(int16_t x, int16_t y, int16_t r, uint16_t color)
{
  display.fillCircle(x, y, r, color);
}

void drawDashedCircle(int16_t x, int16_t y, int16_t r, int16_t dashLength, int16_t gapLength, uint16_t color)
{
  if (r <= 0) return;

  float circumference = 2.0f * PI * (float)r;
  float dashAngle = ((float)dashLength / circumference) * 2.0f * PI;
  float gapAngle = ((float)gapLength / circumference) * 2.0f * PI;
  float step = dashAngle + gapAngle;
  if (step <= 0.0f) return;

  for (float angle = 0.0f; angle < 2.0f * PI; angle += step)
  {
    float endAngle = angle + dashAngle;
    int16_t x1 = x + (int16_t)(r * cosf(angle));
    int16_t y1 = y + (int16_t)(r * sinf(angle));
    int16_t x2 = x + (int16_t)(r * cosf(endAngle));
    int16_t y2 = y + (int16_t)(r * sinf(endAngle));
    display.drawLine(x1, y1, x2, y2, color);
  }
}

void drawArc(int16_t x, int16_t y, int16_t r, float gapCenterAngleRad, int16_t gapLengthPx, uint16_t color)
{
  if (r <= 0) return;

  const float chordLengthPx = 3.0f;
  float chordAngle = chordLengthPx / (float)r;
  if (chordAngle <= 0.0f) return;

  float halfGapAngle = ((float)gapLengthPx / (float)r) / 2.0f;

  for (float angle = 0.0f; angle < 2.0f * PI; angle += chordAngle)
  {
    // Signed distance from this chord's start angle to the gap's center,
    // normalized into (-PI, PI] so the comparison below works regardless
    // of where gapCenterAngleRad falls relative to the 0..2*PI sweep.
    float delta = angle - gapCenterAngleRad;
    while (delta > PI) delta -= 2.0f * PI;
    while (delta < -PI) delta += 2.0f * PI;
    if (delta >= -halfGapAngle && delta <= halfGapAngle)
    {
      continue;
    }

    float endAngle = angle + chordAngle;
    int16_t x1 = x + (int16_t)(r * cosf(angle));
    int16_t y1 = y + (int16_t)(r * sinf(angle));
    int16_t x2 = x + (int16_t)(r * cosf(endAngle));
    int16_t y2 = y + (int16_t)(r * sinf(endAngle));
    display.drawLine(x1, y1, x2, y2, color);
  }
}

void drawLightningBolt(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
  // Two overlapping triangles approximate the classic bolt zigzag: an
  // upper wedge pointing down-left and a lower wedge pointing down-left,
  // overlapping in the middle band.
  display.fillTriangle(x + (int16_t)(w * 0.6f), y,
                        x, y + (int16_t)(h * 0.55f),
                        x + (int16_t)(w * 0.5f), y + (int16_t)(h * 0.55f),
                        color);
  display.fillTriangle(x + (int16_t)(w * 0.4f), y + (int16_t)(h * 0.45f),
                        x + w, y + (int16_t)(h * 0.45f),
                        x + (int16_t)(w * 0.35f), y + h,
                        color);
}

void updateScreen(DrawFunction drawFunc)
{
  // Use paged drawing mode (compatible with any processor)
  // The callback must redraw the full frame each iteration
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    if (drawFunc)
    {
      drawFunc();
    }
  }
  while (display.nextPage());
}

void sleepDisplay()
{
  DEBUG_PRINTLN("sleepDisplay: powering off display...");
  display.powerOff();
  DEBUG_PRINTLN("sleepDisplay: hibernate...");
  display.hibernate();
  DEBUG_PRINTLN("sleepDisplay: done");
}