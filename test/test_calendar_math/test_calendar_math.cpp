#include <unity.h>
#include "calendar_math.h"

void setUp() {}
void tearDown() {}

// A month that starts on Saturday (firstWeekday=6) with 31 days requires a
// 6th row to display its final day -- one more than the 5-row standard grid.
static const int SATURDAY_START_31_DAY_FIRST_WEEKDAY = 6;
static const int SATURDAY_START_31_DAY_DAYS_IN_MONTH = 31;

void test_day_1_visible_in_row0_before_any_scroll()
{
    int row, col;
    getCalendarDayCell(SATURDAY_START_31_DAY_FIRST_WEEKDAY, SATURDAY_START_31_DAY_DAYS_IN_MONTH, /*currentDay=*/1, /*dayNum=*/1, 5, &row, &col);
    TEST_ASSERT_EQUAL(0, row);
    TEST_ASSERT_EQUAL(6, col); // Saturday
}

void test_day_31_hidden_before_the_grid_scrolls()
{
    int row, col;
    // Still day 1 of the month: the opening week hasn't passed yet, so the
    // grid hasn't scrolled, and day 31 would need row index 5 -- out of
    // the 5-row (0-4) grid.
    getCalendarDayCell(SATURDAY_START_31_DAY_FIRST_WEEKDAY, SATURDAY_START_31_DAY_DAYS_IN_MONTH, /*currentDay=*/1, /*dayNum=*/31, 5, &row, &col);
    TEST_ASSERT_EQUAL(-1, row);
    TEST_ASSERT_EQUAL(-1, col);
}

void test_day_31_visible_after_the_grid_scrolls()
{
    int row, col;
    // Once currentDay has moved past day 1 (the only day in the opening
    // week), the grid scrolls up by one row and day 31 becomes visible.
    getCalendarDayCell(SATURDAY_START_31_DAY_FIRST_WEEKDAY, SATURDAY_START_31_DAY_DAYS_IN_MONTH, /*currentDay=*/15, /*dayNum=*/31, 5, &row, &col);
    TEST_ASSERT_EQUAL(4, row);
    TEST_ASSERT_EQUAL(1, col); // Monday
}

void test_day_1_scrolls_out_of_view_once_grid_has_scrolled()
{
    int row, col;
    getCalendarDayCell(SATURDAY_START_31_DAY_FIRST_WEEKDAY, SATURDAY_START_31_DAY_DAYS_IN_MONTH, /*currentDay=*/15, /*dayNum=*/1, 5, &row, &col);
    TEST_ASSERT_EQUAL(-1, row);
    TEST_ASSERT_EQUAL(-1, col);
}

void test_day_8_shifts_up_one_row_once_the_grid_has_scrolled()
{
    int row, col;
    // Day 8 is the Saturday after day 1 -- was row 1 before the scroll,
    // should read as row 0 after it (grid moved up by exactly one row).
    getCalendarDayCell(SATURDAY_START_31_DAY_FIRST_WEEKDAY, SATURDAY_START_31_DAY_DAYS_IN_MONTH, /*currentDay=*/15, /*dayNum=*/8, 5, &row, &col);
    TEST_ASSERT_EQUAL(0, row);
    TEST_ASSERT_EQUAL(6, col);
}

void test_normal_month_never_scrolls()
{
    // Synthetic month: starts Sunday (firstWeekday=0), 30 days -- fits
    // exactly in 5 rows (ceil((0+30)/7) == 5), so rowShift must stay 0
    // for every day, regardless of currentDay.
    int row, col;
    getCalendarDayCell(/*firstWeekday=*/0, /*daysInMonth=*/30, /*currentDay=*/30, /*dayNum=*/1, 5, &row, &col);
    TEST_ASSERT_EQUAL(0, row);
    TEST_ASSERT_EQUAL(0, col);

    getCalendarDayCell(/*firstWeekday=*/0, /*daysInMonth=*/30, /*currentDay=*/30, /*dayNum=*/30, 5, &row, &col);
    TEST_ASSERT_EQUAL(4, row);
    TEST_ASSERT_EQUAL(1, col);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_day_1_visible_in_row0_before_any_scroll);
    RUN_TEST(test_day_31_hidden_before_the_grid_scrolls);
    RUN_TEST(test_day_31_visible_after_the_grid_scrolls);
    RUN_TEST(test_day_1_scrolls_out_of_view_once_grid_has_scrolled);
    RUN_TEST(test_day_8_shifts_up_one_row_once_the_grid_has_scrolled);
    RUN_TEST(test_normal_month_never_scrolls);
    return UNITY_END();
}
