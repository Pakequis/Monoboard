#include "calendar_math.h"
#include <ctime>

void getCalendarMonthInfoForYearMonth(int year, int month0, int* outFirstWeekday, int* outDaysInMonth)
{
  // Weekday of the 1st: build that date and let mktime() normalize
  // tm_wday. Noon avoids landing on a DST transition edge.
  struct tm firstOfMonth = {};
  firstOfMonth.tm_isdst = -1;
  firstOfMonth.tm_year = year - 1900;
  firstOfMonth.tm_mon = month0;
  firstOfMonth.tm_mday = 1;
  firstOfMonth.tm_hour = 12;
  mktime(&firstOfMonth);
  *outFirstWeekday = firstOfMonth.tm_wday;

  // Days in the month: day 0 of next month is the last day of this one --
  // mktime() normalizes the underflow for us, so no leap-year table needed.
  struct tm nextMonthDayZero = {};
  nextMonthDayZero.tm_isdst = -1;
  nextMonthDayZero.tm_year = year - 1900;
  nextMonthDayZero.tm_mon = month0 + 1;
  nextMonthDayZero.tm_mday = 0;
  nextMonthDayZero.tm_hour = 12;
  mktime(&nextMonthDayZero);
  *outDaysInMonth = nextMonthDayZero.tm_mday;
}

void getCalendarDayCell(int firstWeekday, int daysInMonth, int currentDay, int dayNum,
                         int weekRows, int* outRow, int* outCol)
{
  // Months whose first day falls late in the week (e.g. starting Saturday
  // with 31 days) need one more week row for their last 1-2 days than a
  // fixed-size grid holds. Rather than shrinking those days into the
  // existing rows, the grid scrolls up by one row once the opening (first)
  // week has fully passed -- that row is no longer relevant to check, and
  // dropping it frees exactly the space the trailing days need.
  int neededRows = (firstWeekday + daysInMonth + 6) / 7; // ceil division
  int rowShift = 0;
  if (neededRows > weekRows)
  {
    int row0LastDay = 7 - firstWeekday; // last day number still in the opening week
    if (currentDay > row0LastDay) rowShift = 1;
  }

  int cellIndex = firstWeekday + dayNum - 1;
  int row = cellIndex / 7 - rowShift;
  if (row < 0 || row >= weekRows)
  {
    *outRow = -1;
    *outCol = -1;
    return;
  }
  *outRow = row;
  *outCol = cellIndex % 7;
}
