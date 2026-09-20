#include <unity.h>
#include "calendar_math.h"

void setUp() {}
void tearDown() {}

void test_month_starting_saturday_with_31_days()
{
    // Saturday-start, 31-day month -- the case that needs a 6th grid row.
    int firstWeekday, daysInMonth;
    getCalendarMonthInfoForYearMonth(2026, /*month0=*/7, &firstWeekday, &daysInMonth);
    TEST_ASSERT_EQUAL(6, firstWeekday); // Saturday
    TEST_ASSERT_EQUAL(31, daysInMonth);
}

void test_leap_month_with_29_days()
{
    // A leap-year February (29 days).
    int firstWeekday, daysInMonth;
    getCalendarMonthInfoForYearMonth(2028, /*month0=*/1, &firstWeekday, &daysInMonth);
    TEST_ASSERT_EQUAL(29, daysInMonth);
}

void test_nonleap_month_with_28_days()
{
    // A non-leap-year February (28 days).
    int firstWeekday, daysInMonth;
    getCalendarMonthInfoForYearMonth(2027, /*month0=*/1, &firstWeekday, &daysInMonth);
    TEST_ASSERT_EQUAL(28, daysInMonth);
}

void test_december_month_with_31_days_rolls_correctly()
{
    // December has 31 days regardless of year -- also exercises month0+1
    // rolling from 11 (December) to 12, which mktime() must normalize into
    // January of the following year rather than an invalid month index.
    int firstWeekday, daysInMonth;
    getCalendarMonthInfoForYearMonth(2026, /*month0=*/11, &firstWeekday, &daysInMonth);
    TEST_ASSERT_EQUAL(31, daysInMonth);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_month_starting_saturday_with_31_days);
    RUN_TEST(test_leap_month_with_29_days);
    RUN_TEST(test_nonleap_month_with_28_days);
    RUN_TEST(test_december_month_with_31_days_rolls_correctly);
    return UNITY_END();
}
