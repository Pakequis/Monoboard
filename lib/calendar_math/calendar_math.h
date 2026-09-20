#ifndef CALENDAR_MATH_H
#define CALENDAR_MATH_H

// Fills outFirstWeekday (0=Sunday..6=Saturday, matching struct tm::tm_wday)
// and outDaysInMonth with calendar facts about the given year/0-based
// month, using mktime()'s own date normalization instead of a hardcoded
// leap-year table. year is the real calendar year (e.g. 2026, not
// year-1900); month0 is 0-based (0=January, matching struct tm::tm_mon).
void getCalendarMonthInfoForYearMonth(int year, int month0, int* outFirstWeekday, int* outDaysInMonth);

// Returns the (row, col) grid cell for dayNum (1-based) in a calendar grid
// of weekRows rows x 7 columns (0=Sunday..6=Saturday), given the month's
// firstWeekday (0-6) and how many of that grid's rows are currently
// visible. Months whose first day falls late in the week can need more
// rows than a fixed-size grid holds for their last 1-2 days -- once
// currentDay has moved past the opening (first) week, the grid scrolls up
// by one row, freeing space for those trailing days instead of dropping
// them. Sets *outRow = *outCol = -1 if dayNum's cell has scrolled past or
// isn't revealed yet at the given currentDay.
void getCalendarDayCell(int firstWeekday, int daysInMonth, int currentDay, int dayNum,
                         int weekRows, int* outRow, int* outCol);

#endif // CALENDAR_MATH_H
