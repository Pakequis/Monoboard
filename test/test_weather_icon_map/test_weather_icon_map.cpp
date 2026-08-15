#include <unity.h>
#include "weather_icon_map.h"

void setUp() {}
void tearDown() {}

void test_clear_sky_day()
{
    TEST_ASSERT_EQUAL('1', weatherCodeToIconChar(0, 1));
}

void test_clear_sky_night()
{
    TEST_ASSERT_EQUAL('8', weatherCodeToIconChar(0, 0));
}

void test_partly_cloudy_day()
{
    TEST_ASSERT_EQUAL('9', weatherCodeToIconChar(1, 1));
    TEST_ASSERT_EQUAL('9', weatherCodeToIconChar(2, 1));
}

void test_partly_cloudy_night()
{
    TEST_ASSERT_EQUAL(';', weatherCodeToIconChar(1, 0));
    TEST_ASSERT_EQUAL(';', weatherCodeToIconChar(2, 0));
}

void test_overcast_and_fog_ignore_day_night()
{
    TEST_ASSERT_EQUAL('2', weatherCodeToIconChar(3, 1));
    TEST_ASSERT_EQUAL('2', weatherCodeToIconChar(45, 0));
    TEST_ASSERT_EQUAL('2', weatherCodeToIconChar(48, 1));
}

void test_thunderstorm_variants()
{
    TEST_ASSERT_EQUAL('6', weatherCodeToIconChar(95, 1));
    TEST_ASSERT_EQUAL('7', weatherCodeToIconChar(96, 0));
    TEST_ASSERT_EQUAL('7', weatherCodeToIconChar(99, 1));
}

void test_unknown_code_returns_blank()
{
    TEST_ASSERT_EQUAL(':', weatherCodeToIconChar(-1, 1));
    TEST_ASSERT_EQUAL(':', weatherCodeToIconChar(1000, 0));
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_clear_sky_day);
    RUN_TEST(test_clear_sky_night);
    RUN_TEST(test_partly_cloudy_day);
    RUN_TEST(test_partly_cloudy_night);
    RUN_TEST(test_overcast_and_fog_ignore_day_night);
    RUN_TEST(test_thunderstorm_variants);
    RUN_TEST(test_unknown_code_returns_blank);
    return UNITY_END();
}
