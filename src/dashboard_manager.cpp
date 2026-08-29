/*
  Dashboard Manager - Implementation
  Handles all dashboard drawing operations
*/

#include "dashboard_manager.h"
#include "config.h"
#include "display_manager.h"
#include "time_manager.h"
#include "wifi_manager.h"
#include "content_manager.h"
#include "local_sensors.h"
#include "strings.h"
#include "calendar_math.h"
#include "text_layout.h"
#include <math.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include "dseg7_classic_bold.h"
#include "weathericons_32.h"

void initDashboard()
{
  // Nothing to initialize for now
}

void drawDashboard()
{
  updateScreen([]{
    clearScreen();

    // Outer border, inset from the physical screen edges
    drawRect(DISPLAY_BORDER_OFFSET, DISPLAY_BORDER_OFFSET,
             display.width() - 2 * DISPLAY_BORDER_OFFSET,
             display.height() - 2 * DISPLAY_BORDER_OFFSET,
             GxEPD_BLACK);

    // Draw header separator below the single header line
    drawLine(DISPLAY_FRAME_MARGIN, HEADER_SEPARATOR_Y, display.width() - DISPLAY_FRAME_MARGIN, HEADER_SEPARATOR_Y, GxEPD_BLACK);

    // Header: title on the left; WiFi status and firmware version on the
    // right. The clock lives in the bottom-right quadrant instead (see the
    // bottom-right block further down). statusLine remains a single string,
    // so its contents stay together when the WiFi state changes.
    const char* wifiStatus = isWifiConnected() ? STR_WIFI_OK : STR_WIFI_ERROR;
    char statusLine[32];
    snprintf(statusLine, sizeof(statusLine), "%s  %s", wifiStatus, FIRMWARE_VERSION);
    int16_t headerInset = DISPLAY_FRAME_MARGIN + HEADER_TEXT_X_OFFSET;
    writeText(headerInset, HEADER_TITLE_Y, STR_DASHBOARD_TITLE, &FreeMonoBold9pt7b, GxEPD_BLACK);
    int16_t statusWidth = getTextWidth(statusLine, &FreeMonoBold9pt7b);
    int16_t statusX = display.width() - headerInset - statusWidth;
    writeText(statusX, HEADER_TITLE_Y, statusLine, &FreeMonoBold9pt7b, GxEPD_BLACK);

    // ===== Content area: 2x2 grid =====
    // Top row (left to right): weather forecast grid, lightning distance
    // box, calendar. Bottom row: news headlines (left, wider), clock +
    // crypto/FX box (right, directly under the calendar, same width --
    // their shared left edge is derived from the calendar's own left
    // edge below instead of a hardcoded constant). No standalone divider
    // lines anywhere in the grid (see config.h's "Content Area Layout"
    // section) -- every quadrant is its own bordered box (except the
    // bottom-right one, split into a clock box and a crypto/FX box), with a
    // CONTENT_BOX_MARGIN gap (both axes) providing the visual
    // separation from its neighbors.

    // Shared vertical bounds for all three top-row boxes.
    int16_t topRowY0 = HEADER_SEPARATOR_Y + CONTENT_BOX_MARGIN;
    int16_t topRowY1 = CONTENT_MID_Y - CONTENT_BOX_MARGIN;
    int16_t topRowHeight = topRowY1 - topRowY0;

    // Weather grid's left edge/total width, hoisted here (ahead of the
    // grid's own drawing code further down) since the lightning box's
    // left edge is now derived directly from them instead of a separate
    // standalone divider constant positioned between the two boxes.
    int16_t wgridX0 = DISPLAY_FRAME_MARGIN + CONTENT_BOX_MARGIN;
    int16_t wgridTotalWidth = WEATHER_GRID_COLUMNS * WEATHER_BOX_COLUMN_WIDTH
                             + (WEATHER_GRID_COLUMNS - 1) * WEATHER_BOX_GAP;

    // Calendar's left edge, hoisted here (ahead of its own drawing code
    // further down) since the clock and reserved boxes' own shared left
    // edge derives from it directly -- that quadrant sits right under
    // the calendar, same width.
    int16_t calX0 = display.width() - DISPLAY_FRAME_MARGIN - CONTENT_BOX_MARGIN - CALENDAR_WIDTH;

    // Bottom row: news box (left) and clock+crypto/FX boxes (right), same
    // margin-from-CONTENT_MID_Y treatment the top row already gets from
    // HEADER_SEPARATOR_Y -- both rows are now fully composed of
    // independent bordered boxes with a CONTENT_BOX_MARGIN gap between
    // them, no standalone divider line drawn anywhere in the grid.
    int16_t bottomRowY0 = CONTENT_MID_Y;
    // DISPLAY_BORDER_OFFSET (not DISPLAY_FRAME_MARGIN) here matches this
    // gap exactly to HEADER_SEPARATOR_Y -> topRowY0's: both measure
    // CONTENT_BOX_MARGIN from a real drawn border line (the outer frame
    // rect's own bottom edge here, the header separator up top), not
    // from the wider general-purpose content margin used on the left/
    // right edges.
    int16_t bottomRowY1 = display.height() - DISPLAY_BORDER_OFFSET - CONTENT_BOX_MARGIN;
    int16_t cryptoBoxX0 = calX0;
    int16_t newsBoxX1 = cryptoBoxX0 - CONTENT_BOX_MARGIN;

    // ---- Top row, box 3 (rightmost): calendar ----
    // Right-anchored -- see config.h's comment on CALENDAR_WIDTH's box for why.
    int16_t calY0 = topRowY0;
    int16_t calHeight = topRowHeight;
    drawRect(calX0, calY0, CALENDAR_WIDTH, calHeight, GxEPD_BLACK);

    // ---- Top row, box 2 (middle): lightning ("Raios") distance box ----
    // Real distance scale -- see config.h's comment on LIGHTNING_MAX_KM.
    // Dashed rings at round-number km values, each labeled;
    // confirmed strikes (getStrikeHistoryCount()/getStrikeHistoryEntry(),
    // local_sensors.h) overlay as solid, thick rings at their real
    // distance, thickness tiered by relative energy. Header shows the
    // strike rate (getLightningRateText(), strikes/hour) in the same
    // white-on-black bar used before (matching the calendar's "negativo"
    // title bar).
    int16_t lightningX0 = wgridX0 + wgridTotalWidth + CONTENT_BOX_MARGIN;
    int16_t lightningY0 = topRowY0;
    int16_t lightningX1 = calX0 - CONTENT_BOX_MARGIN;
    int16_t lightningY1 = topRowY1;
    int16_t lightningWidth = lightningX1 - lightningX0;
    drawRect(lightningX0, lightningY0, lightningWidth, lightningY1 - lightningY0, GxEPD_BLACK);

    fillRect(lightningX0, lightningY0, lightningWidth, LIGHTNING_HEADER_HEIGHT, GxEPD_BLACK);
    char lightningRate[LIGHTNING_STATUS_TEXT_LEN];
    getLightningRateText(lightningRate, sizeof(lightningRate));
    char lightningLabel[LIGHTNING_STATUS_TEXT_LEN + 12];
    snprintf(lightningLabel, sizeof(lightningLabel), "%s: %s/h", STR_LABEL_LIGHTNING, lightningRate);
    int16_t lightningLabelWidth = getTextWidth(lightningLabel, &FreeMonoBold9pt7b);
    writeText(lightningX0 + (lightningWidth - lightningLabelWidth) / 2, lightningY0 + LIGHTNING_HEADER_LABEL_Y_OFFSET,
              lightningLabel, &FreeMonoBold9pt7b, GxEPD_WHITE);

    // Alert icon: a strong strike landed within LIGHTNING_CLOSE_KM_THRESHOLD
    // since the last redraw. Drawn white to show up on the header's
    // black bar, same as the label text above.
    if (isLightningAlertActive())
    {
      int16_t iconX = lightningX1 - LIGHTNING_ALERT_ICON_MARGIN - LIGHTNING_ALERT_ICON_SIZE;
      int16_t iconY = lightningY0 + (LIGHTNING_HEADER_HEIGHT - LIGHTNING_ALERT_ICON_SIZE) / 2;
      drawLightningBolt(iconX, iconY, LIGHTNING_ALERT_ICON_SIZE, LIGHTNING_ALERT_ICON_SIZE, GxEPD_WHITE);
    }

    int16_t lightningRingsTop = lightningY0 + LIGHTNING_HEADER_HEIGHT;
    int16_t lightningCx = (lightningX0 + lightningX1) / 2;
    int16_t lightningCy = (lightningRingsTop + lightningY1) / 2;
    int16_t lightningRingsHeight = lightningY1 - lightningRingsTop;
    int16_t lightningMaxR = min(lightningWidth, lightningRingsHeight) / 2 - 6;

    // Adaptive ruler range: when every strike currently in history is
    // within LIGHTNING_CLOSE_KM_THRESHOLD, zoom the scale to that range so
    // near strikes spread across the box instead of bunching at the
    // center. A single strike beyond it (kept in history until
    // STRIKE_RESET_TIMEOUT_SEC of silence) snaps the ruler back to the
    // full LIGHTNING_MAX_KM range. The rings always sit at even quarters
    // of the box radius -- only their km labels change.
    uint8_t strikeCount = getStrikeHistoryCount();
    uint8_t lightningScaleMaxKm = LIGHTNING_MAX_KM;
    if (strikeCount > 0)
    {
      lightningScaleMaxKm = LIGHTNING_CLOSE_KM_THRESHOLD;
      for (uint8_t i = 0; i < strikeCount; i++)
      {
        uint8_t km = 0;
        uint32_t energy = 0;
        getStrikeHistoryEntry(i, &km, &energy);
        if (km > LIGHTNING_CLOSE_KM_THRESHOLD)
        {
          lightningScaleMaxKm = LIGHTNING_MAX_KM;
          break;
        }
      }
    }

    float lightningLabelAngleRad = LIGHTNING_RING_LABEL_ANGLE_DEG * PI / 180.0f;
    for (int ring = 0; ring < LIGHTNING_RING_COUNT; ring++)
    {
      int16_t ringR = (int16_t)(lightningMaxR * ((float)(ring + 1) / LIGHTNING_RING_COUNT));
      drawDashedCircle(lightningCx, lightningCy, ringR, LIGHTNING_DASH_LENGTH, LIGHTNING_DASH_GAP, GxEPD_BLACK);

      char kmLabel[6];
      float ringKm = lightningScaleMaxKm * (float)(ring + 1) / LIGHTNING_RING_COUNT;
      snprintf(kmLabel, sizeof(kmLabel), "%g", ringKm);
      int16_t labelX = lightningCx + (int16_t)(ringR * cosf(lightningLabelAngleRad));
      int16_t labelY = lightningCy + (int16_t)(ringR * sinf(lightningLabelAngleRad));
      writeText(labelX, labelY, kmLabel, &FreeMonoBold9pt7b, GxEPD_BLACK);
    }
    fillCircle(lightningCx, lightningCy, 2, GxEPD_BLACK);

    // Unit for the ring labels above -- kept off the ruler itself (all 4
    // stay bare numbers) and tucked in the box's bottom-left corner
    // instead, clear of both the rings (bounded to lightningMaxR, inscribed
    // well short of the corners) and the box border.
    writeText(lightningX0 + LIGHTNING_UNIT_LABEL_X_OFFSET, lightningY1 - LIGHTNING_UNIT_LABEL_Y_OFFSET,
              "km", &FreeMonoBold9pt7b, GxEPD_BLACK);

    for (uint8_t i = 0; i < strikeCount; i++)
    {
      uint8_t strikeKm = 0;
      uint32_t strikeEnergy = 0;
      getStrikeHistoryEntry(i, &strikeKm, &strikeEnergy);

      int8_t thickness = (strikeEnergy >= LIGHTNING_ENERGY_HIGH_THRESHOLD) ? 2
                        : (strikeEnergy >= LIGHTNING_ENERGY_MEDIUM_THRESHOLD) ? 1
                        : 0;

      if (strikeKm <= LIGHTNING_OVERHEAD_KM)
      {
        // The AS3935's "storm overhead" reading (register value 1): a ring
        // would collapse under the center marker, so draw a filled disc
        // instead -- unmistakably "right on top of us", visually distinct
        // from a merely close strike's ring.
        fillCircle(lightningCx, lightningCy, LIGHTNING_OVERHEAD_DISC_R + thickness, GxEPD_BLACK);
        continue;
      }

      // Linear km->radius against the current ruler range, clamped both
      // ends: the far end can't spill past the outer ring, and the near
      // end can't collapse into the center dot (LIGHTNING_STRIKE_MIN_R).
      int16_t strikeR = (int16_t)(lightningMaxR * ((float)strikeKm / lightningScaleMaxKm));
      if (strikeR > lightningMaxR)
      {
        strikeR = lightningMaxR;
      }
      if (strikeR < LIGHTNING_STRIKE_MIN_R)
      {
        strikeR = LIGHTNING_STRIKE_MIN_R;
      }

      if (strikeKm < LIGHTNING_CLOSE_KM_THRESHOLD)
      {
        // Close enough to draw the full circle instead of a gapped arc --
        // reads as "very close" more clearly, even if it overlaps a
        // ring's own label.
        for (int8_t offset = -thickness; offset <= thickness; offset++)
        {
          drawCircle(lightningCx, lightningCy, strikeR + offset, GxEPD_BLACK);
        }
      }
      else
      {
        // Arc, not a full circle: a solid ring at the same radius as one of
        // the 4 ruler distances would otherwise paint straight over that
        // ring's number label. Gap is centered on the same fixed direction
        // the labels themselves sit along (LIGHTNING_RING_LABEL_ANGLE_DEG).
        for (int8_t offset = -thickness; offset <= thickness; offset++)
        {
          drawArc(lightningCx, lightningCy, strikeR + offset, lightningLabelAngleRad, LIGHTNING_RING_LABEL_GAP_PX, GxEPD_BLACK);
        }
      }
    }

    // ---- Top row, box 3: calendar content (box rect already drawn above) ----
    int firstWeekday = 0;
    int daysInMonth = 0;
    getCalendarMonthInfo(&firstWeekday, &daysInMonth);

    // daysInMonth == 0 means the epoch isn't valid yet (no NTP sync since
    // boot) -- leave the day grid blank rather than drawing a wrong month,
    // same placeholder rule as the header clock/date.
    if (daysInMonth > 0)
    {
      time_t nowEpoch = getCurrentTime();
      const struct tm* nowTm = localtime(&nowEpoch);
      int currentDay = nowTm->tm_mday;

      // Month/year header bar, white-on-black ("negativo").
      fillRect(calX0, calY0, CALENDAR_WIDTH, CALENDAR_HEADER_HEIGHT, GxEPD_BLACK);
      char calTitle[24];
      snprintf(calTitle, sizeof(calTitle), "%s %d", STR_MONTHS[nowTm->tm_mon], nowTm->tm_year + 1900);
      int16_t calTitleWidth = getTextWidth(calTitle, &FreeMonoBold9pt7b);
      writeText(calX0 + (CALENDAR_WIDTH - calTitleWidth) / 2, calY0 + CALENDAR_TITLE_LABEL_Y_OFFSET,
                calTitle, &FreeMonoBold9pt7b, GxEPD_WHITE);

      // Weekday letters row.
      float calColWidth = CALENDAR_WIDTH / 7.0f;
      int16_t weekdayRowTop = calY0 + CALENDAR_HEADER_HEIGHT;
      for (int col = 0; col < 7; col++)
      {
        int16_t colCx = calX0 + (int16_t)(calColWidth * col + calColWidth / 2);
        int16_t dw = getTextWidth(STR_WEEKDAYS_SHORT[col], &FreeMonoBold9pt7b);
        writeText(colCx - dw / 2, weekdayRowTop + CALENDAR_WEEKDAY_LABEL_Y_OFFSET,
                  STR_WEEKDAYS_SHORT[col], &FreeMonoBold9pt7b, GxEPD_BLACK);
      }

      // Week rows, evenly spaced across the remaining height so there's no
      // leftover blank gap below the last row.
      int16_t weeksTop = weekdayRowTop + CALENDAR_WEEKDAY_HEIGHT;
      float weeksAvailableHeight = calHeight - CALENDAR_HEADER_HEIGHT - CALENDAR_WEEKDAY_HEIGHT;
      float rowHeight = weeksAvailableHeight / CALENDAR_WEEK_ROWS;

      // Grid lines -- a full table look across the weekday-letters row and
      // every week row (internal dividers only; the box's own border
      // already draws the outer edge).
      for (int col = 1; col < 7; col++)
      {
        int16_t lineX = calX0 + (int16_t)(calColWidth * col);
        // -1: drawRect()'s own border sits at calY0+calHeight-1 (width/height
        // are pixel counts, not an endpoint coordinate); matching that exactly
        // keeps this divider from overshooting 1px past the bottom border.
        drawLine(lineX, weekdayRowTop, lineX, calY0 + calHeight - 1, GxEPD_BLACK);
      }
      for (int row = 0; row <= CALENDAR_WEEK_ROWS; row++)
      {
        int16_t lineY = weeksTop + (int16_t)(rowHeight * row);
        // -1: same reasoning as the vertical dividers above, but against the
        // box's right border (calX0+CALENDAR_WIDTH-1) instead of its bottom.
        drawLine(calX0, lineY, calX0 + CALENDAR_WIDTH - 1, lineY, GxEPD_BLACK);
      }

      for (int dayNum = 1; dayNum <= daysInMonth; dayNum++)
      {
        int row, col;
        getCalendarDayCell(firstWeekday, daysInMonth, currentDay, dayNum, CALENDAR_WEEK_ROWS, &row, &col);
        if (row < 0) continue; // scrolled past, or not yet revealed

        int16_t rowTop = weeksTop + (int16_t)(rowHeight * row);
        int16_t rowH = (int16_t)rowHeight;
        int16_t colX = calX0 + (int16_t)(calColWidth * col);
        int16_t colW = (int16_t)calColWidth;
        int16_t colCx = colX + colW / 2;

        char dayBuf[3];
        snprintf(dayBuf, sizeof(dayBuf), "%02d", dayNum);

        // Today: filled black cell ("negativo"), not a border square --
        // matches the header bars' style instead of standing apart.
        uint16_t dayColor = GxEPD_BLACK;
        if (dayNum == currentDay)
        {
          fillRect(colX, rowTop, colW, rowH, GxEPD_BLACK);
          dayColor = GxEPD_WHITE;
        }
        int16_t dw = getTextWidth(dayBuf, &FreeMonoBold9pt7b);
        // Last column only: nudge 1px left. writeText()'s cursor-based
        // centering doesn't account for the font glyph's left-side bearing,
        // so the ink drifts slightly right of center in every column -- the
        // inner columns have a grid line to absorb that, but the last one
        // sits flush against the box's own right border with no slack.
        int16_t dayTextX = colCx - dw / 2 - (col == 6 ? 1 : 0);
        writeText(dayTextX, rowTop + CALENDAR_DAY_LABEL_Y_OFFSET, dayBuf, &FreeMonoBold9pt7b, dayColor);
      }
    }

    // ---- Top row, box 1: temp/humidity + weather forecast grid (leftmost) ----
    // Leftmost -- this box's width is what the lightning box's left edge
    // is derived from (wgridX0/wgridTotalWidth, hoisted above, near the
    // top-row shared bounds).
    int16_t wgridY0 = topRowY0;

    fillRect(wgridX0, wgridY0, wgridTotalWidth, WEATHER_SENSOR_LINE_HEIGHT, GxEPD_BLACK);

    char tempHumidityText[TEMP_HUMIDITY_TEXT_LEN];
    getTempHumidityText(tempHumidityText, sizeof(tempHumidityText));
    int16_t tempHumidityWidth = getTextWidth(tempHumidityText, &FreeMonoBold9pt7b);
    int16_t tempHumidityX = wgridX0 + (wgridTotalWidth - tempHumidityWidth) / 2;
    writeText(tempHumidityX, wgridY0 + WEATHER_SENSOR_LINE_Y_OFFSET, tempHumidityText, &FreeMonoBold9pt7b, GxEPD_WHITE);

    // Degree mark drawn as a circle (FreeMonoBold9pt7b has no "°" glyph),
    // positioned right after the temperature number's ink width. White on
    // this bar's black fill, unlike every other degree mark in the app.
    char sensorTempNumBuf[8];
    getTemperatureNumberText(sensorTempNumBuf, sizeof(sensorTempNumBuf));
    int16_t sensorTempNumWidth = getTextWidth(sensorTempNumBuf, &FreeMonoBold9pt7b);
    drawCircle(tempHumidityX + sensorTempNumWidth + DEGREE_CIRCLE_X_OFFSET,
               wgridY0 + WEATHER_SENSOR_LINE_Y_OFFSET - DEGREE_CIRCLE_Y_OFFSET, DEGREE_CIRCLE_RADIUS, GxEPD_WHITE);

    int16_t wgridTop = wgridY0 + WEATHER_SENSOR_LINE_HEIGHT;
    int16_t wgridAvailableHeight = calHeight - WEATHER_SENSOR_LINE_HEIGHT;
    int16_t weatherBoxHeight = (wgridAvailableHeight - WEATHER_BOX_GAP) / WEATHER_GRID_ROWS;

    for (size_t hour = 0; hour < WEATHER_FORECAST_HOURS; hour++)
    {
      int col = hour % WEATHER_GRID_COLUMNS;
      int row = hour / WEATHER_GRID_COLUMNS;
      int16_t boxX = wgridX0 + col * (WEATHER_BOX_COLUMN_WIDTH + WEATHER_BOX_GAP);
      int16_t boxY = wgridTop + row * (weatherBoxHeight + WEATHER_BOX_GAP);
      drawRect(boxX, boxY, WEATHER_BOX_COLUMN_WIDTH, weatherBoxHeight, GxEPD_BLACK);

      char hourLabel[6];
      getWeatherForecastHourLabel(hour, hourLabel, sizeof(hourLabel));
      int16_t hourWidth = getTextWidth(hourLabel, &FreeMonoBold9pt7b);
      writeText(boxX + (WEATHER_BOX_COLUMN_WIDTH - hourWidth) / 2, boxY + WEATHER_HOUR_LABEL_Y_OFFSET,
                hourLabel, &FreeMonoBold9pt7b, GxEPD_BLACK);

      // Degree mark drawn as a circle (same reason as the sensor line's
      // temperature), positioned right after the temperature number's ink
      // width, before the "C".
      char tempNumLabel[6];
      getWeatherForecastTemperatureNumberLabel(hour, tempNumLabel, sizeof(tempNumLabel));
      char tempLabel[8];
      snprintf(tempLabel, sizeof(tempLabel), "%s C", tempNumLabel);
      int16_t tempWidth = getTextWidth(tempLabel, &FreeMonoBold9pt7b);
      int16_t tempX = boxX + (WEATHER_BOX_COLUMN_WIDTH - tempWidth) / 2;
      int16_t tempY = boxY + WEATHER_TEMP_LABEL_Y_OFFSET;
      writeText(tempX, tempY, tempLabel, &FreeMonoBold9pt7b, GxEPD_BLACK);

      int16_t tempNumWidth = getTextWidth(tempNumLabel, &FreeMonoBold9pt7b);
      int16_t weatherDegreeCircleX = tempX + tempNumWidth + DEGREE_CIRCLE_X_OFFSET;
      int16_t weatherDegreeCircleY = tempY - DEGREE_CIRCLE_Y_OFFSET;
      drawCircle(weatherDegreeCircleX, weatherDegreeCircleY, DEGREE_CIRCLE_RADIUS, GxEPD_BLACK);

      const char iconStr[2] = { getWeatherForecastIconChar(hour), '\0' };
      int16_t iconWidth = getTextWidth(iconStr, &WeatherIcons_32);
      writeText(boxX + (WEATHER_BOX_COLUMN_WIDTH - iconWidth) / 2, boxY + WEATHER_ICON_Y_OFFSET,
                iconStr, &WeatherIcons_32, GxEPD_BLACK);
    }

    // ---- Bottom-left: news headlines (word-wrapped across up to 2 lines each, carousel) ----
    int16_t newsBoxX0 = DISPLAY_FRAME_MARGIN + CONTENT_BOX_MARGIN;
    drawRect(newsBoxX0, bottomRowY0, newsBoxX1 - newsBoxX0, bottomRowY1 - bottomRowY0, GxEPD_BLACK);

    int16_t newsX = newsBoxX0 + CONTENT_TEXT_X_OFFSET;
    int16_t newsMaxWidth = newsBoxX1 - CONTENT_BOX_MARGIN - newsX;
    for (size_t i = 0; i < NEWS_HEADLINES_PER_BUFFER; i++)
    {
      char headline[NEWS_HEADLINE_LEN];
      bool isRealHeadline = getNewsHeadline(i, headline, sizeof(headline));
      if (headline[0] == '\0') continue;

      // fullLine/line1/line2 below can hold up to NEWS_HEADLINE_LEN+16-1 chars;
      // text_layout's internal scratch buffer must be large enough for that
      // many characters plus "..." plus a null terminator, or a candidate
      // string could be silently truncated before its width is measured.
      static_assert(NEWS_HEADLINE_LEN + 19 <= TEXT_LAYOUT_SCRATCH_LEN,
                    "text_layout's scratch buffer is too small for NEWS_HEADLINE_LEN");
      char fullLine[NEWS_HEADLINE_LEN + 16];
      if (isRealHeadline)
      {
        snprintf(fullLine, sizeof(fullLine), "%s %s", NEWS_SOURCE_TAG, headline);
      }
      else
      {
        snprintf(fullLine, sizeof(fullLine), "%s", headline);
      }

      char line1[NEWS_HEADLINE_LEN + 16];
      char line2[NEWS_HEADLINE_LEN + 16];
      wrapToTwoLines(fullLine, line1, sizeof(line1), line2, sizeof(line2),
                      [](const char* s) -> int16_t { return getTextWidth(s, &FreeMonoBold9pt7b); },
                      newsMaxWidth);

      int16_t headlineBaseline = CONTENT_MID_Y + NEWS_FIRST_LINE_Y_OFFSET + (int16_t)i * 2 * NEWS_LINE_HEIGHT;
      writeText(newsX, headlineBaseline, line1, &FreeMonoBold9pt7b, GxEPD_BLACK);
      if (line2[0] != '\0')
      {
        writeText(newsX, headlineBaseline + NEWS_LINE_HEIGHT, line2, &FreeMonoBold9pt7b, GxEPD_BLACK);
      }
    }

    // ---- Bottom-right: clock box (top) + crypto/FX box (below, BTC/ETH/
    // USD-BRL quotes -- see content_manager.h's getCrypto*Text()),
    // directly under the calendar, same width -- two separate boxes
    // stacked with the usual CONTENT_BOX_MARGIN gap, not one shared box. ----
    int16_t clockBoxY0 = bottomRowY0;
    drawRect(cryptoBoxX0, clockBoxY0, CALENDAR_WIDTH, CLOCK_BOX_HEIGHT, GxEPD_BLACK);

    int16_t cryptoBoxY0 = clockBoxY0 + CLOCK_BOX_HEIGHT + CONTENT_BOX_MARGIN;
    drawRect(cryptoBoxX0, cryptoBoxY0, CALENDAR_WIDTH, bottomRowY1 - cryptoBoxY0, GxEPD_BLACK);

    char timeBuf[6];
    getTimeString(timeBuf, sizeof(timeBuf));
    writeTextCenteredInBox(cryptoBoxX0, CALENDAR_WIDTH, clockBoxY0 + CLOCK_LINE_Y_OFFSET, timeBuf,
                           &DSEG7_Classic_Bold_44, GxEPD_BLACK);

    int16_t cryptoX = cryptoBoxX0 + CONTENT_TEXT_X_OFFSET;
    char cryptoLine[CRYPTO_LINE_TEXT_LEN];

    getCryptoBitcoinText(cryptoLine, sizeof(cryptoLine));
    writeText(cryptoX, cryptoBoxY0 + CRYPTO_FIRST_LINE_Y_OFFSET, cryptoLine, &FreeMonoBold9pt7b, GxEPD_BLACK);

    getCryptoEthereumText(cryptoLine, sizeof(cryptoLine));
    writeText(cryptoX, cryptoBoxY0 + CRYPTO_FIRST_LINE_Y_OFFSET + CRYPTO_LINE_HEIGHT, cryptoLine, &FreeMonoBold9pt7b, GxEPD_BLACK);

    getCryptoFxText(cryptoLine, sizeof(cryptoLine));
    writeText(cryptoX, cryptoBoxY0 + CRYPTO_FIRST_LINE_Y_OFFSET + 2 * CRYPTO_LINE_HEIGHT, cryptoLine, &FreeMonoBold9pt7b, GxEPD_BLACK);
  });
}
