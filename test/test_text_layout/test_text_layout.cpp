#include <unity.h>
#include "text_layout.h"
#include <cstring>

void setUp() {}
void tearDown() {}

// Deterministic stand-in for a real font's getTextWidth(): every ASCII
// character is exactly 10px wide, so word-wrap decisions in these tests
// are simple, verifiable arithmetic instead of depending on real glyph
// metrics.
static const int16_t FIXED_CHAR_WIDTH = 10;

static int16_t fixedWidthMeasurer(const char* text)
{
    return static_cast<int16_t>(strlen(text) * FIXED_CHAR_WIDTH);
}

void test_wrap_leaves_short_text_on_one_line()
{
    char line1[32], line2[32];
    // "Short text" is 10 chars = 100px, fits in maxWidth=150.
    wrapToTwoLines("Short text", line1, sizeof(line1), line2, sizeof(line2), fixedWidthMeasurer, 150);
    TEST_ASSERT_EQUAL_STRING("Short text", line1);
    TEST_ASSERT_EQUAL_STRING("", line2);
}

void test_wrap_splits_at_the_last_word_boundary_that_fits()
{
    char line1[32], line2[32];
    // "one two three four" -- maxWidth=140px allows up to 14 chars per
    // line. "one two three" is 13 chars (130px, fits); adding " four"
    // would be 18 chars (180px, doesn't). Split after "three".
    wrapToTwoLines("one two three four", line1, sizeof(line1), line2, sizeof(line2), fixedWidthMeasurer, 140);
    TEST_ASSERT_EQUAL_STRING("one two three", line1);
    TEST_ASSERT_EQUAL_STRING("four", line2);
}

void test_wrap_hard_truncates_a_single_word_too_wide_for_one_line()
{
    char line1[32], line2[32];
    // "Supercalifragilistic" (20 chars = 200px) has no space at all, and
    // is wider than maxWidth=100px (10 chars) -- falls back to a hard
    // truncateToFitWidth() cut with "..." appended, and line2 stays empty.
    // The truncate loop keeps the longest prefix whose "<prefix>..." still
    // fits: prefix length + 3 (the ellipsis) must be at most 10 chars, so
    // the longest fitting prefix is 7 chars ("Superca"), not 6.
    wrapToTwoLines("Supercalifragilistic", line1, sizeof(line1), line2, sizeof(line2), fixedWidthMeasurer, 100);
    TEST_ASSERT_EQUAL_STRING("Superca...", line1);
    TEST_ASSERT_EQUAL_STRING("", line2);
}

void test_wrap_truncates_a_too_long_second_line_too()
{
    char line1[16], line2[16];
    // First word boundary fits line1 within maxWidth; the remainder is
    // still too long for line2 at the same maxWidth and gets truncated.
    // Same maxWidth=100 as above, so the same 7-char-prefix-plus-ellipsis
    // rule applies: "defghij..." (7 chars kept), not "defghi...".
    wrapToTwoLines("abc defghijklmnopqrst", line1, sizeof(line1), line2, sizeof(line2), fixedWidthMeasurer, 100);
    TEST_ASSERT_EQUAL_STRING("abc", line1);
    TEST_ASSERT_EQUAL_STRING("defghij...", line2);
}

void test_truncate_appends_ellipsis_when_over_width()
{
    char text[32];
    strcpy(text, "abcdefghij"); // 10 chars = 100px
    truncateToFitWidth(text, sizeof(text), fixedWidthMeasurer, 70); // fits 7 chars
    TEST_ASSERT_EQUAL_STRING("abcd...", text);
}

void test_truncate_leaves_text_that_already_fits_alone()
{
    char text[32];
    strcpy(text, "short");
    truncateToFitWidth(text, sizeof(text), fixedWidthMeasurer, 100);
    TEST_ASSERT_EQUAL_STRING("short", text);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_wrap_leaves_short_text_on_one_line);
    RUN_TEST(test_wrap_splits_at_the_last_word_boundary_that_fits);
    RUN_TEST(test_wrap_hard_truncates_a_single_word_too_wide_for_one_line);
    RUN_TEST(test_wrap_truncates_a_too_long_second_line_too);
    RUN_TEST(test_truncate_appends_ellipsis_when_over_width);
    RUN_TEST(test_truncate_leaves_text_that_already_fits_alone);
    return UNITY_END();
}
