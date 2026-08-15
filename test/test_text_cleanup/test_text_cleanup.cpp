#include <unity.h>
#include "text_cleanup.h"
#include <cstring>

void setUp() {}
void tearDown() {}

void test_strip_source_suffix_removes_trailing_source_name()
{
    char text[64];
    strcpy(text, "Some Real Headline - The News Source");
    stripSourceSuffix(text);
    TEST_ASSERT_EQUAL_STRING("Some Real Headline", text);
}

void test_strip_source_suffix_leaves_text_without_a_suffix_alone()
{
    char text[64];
    strcpy(text, "No Suffix Here");
    stripSourceSuffix(text);
    TEST_ASSERT_EQUAL_STRING("No Suffix Here", text);
}

void test_decode_entities_handles_all_known_entities()
{
    char out[128];
    decodeEntities("Cats &amp; Dogs &lt;3 &gt; &quot;quoted&quot; &#39;s&apos;", out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING("Cats & Dogs <3 > \"quoted\" 's'", out);
}

void test_decode_entities_passes_through_unrecognized_sequences()
{
    char out[64];
    decodeEntities("A &weird; entity", out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING("A &weird; entity", out);
}

void test_transliterate_converts_latin1_accents()
{
    char out[64];
    // "Sao Paulo" with a tilde-a, UTF-8 encoded: 'S','\xC3','\xA3','o', ...
    const char in[] = "S\xC3\xA3o Jo\xC3\xA3o";
    transliterate(in, out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING("Sao Joao", out);
}

void test_transliterate_converts_smart_quotes_and_dashes()
{
    char out[64];
    // U+2018/U+2019 (smart single quotes) and U+2013 (en dash), UTF-8.
    const char in[] = "\xE2\x80\x98quoted\xE2\x80\x99 \xE2\x80\x93 dashed";
    transliterate(in, out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING("'quoted' - dashed", out);
}

void test_transliterate_passes_through_plain_ascii()
{
    char out[64];
    transliterate("Plain ASCII text 123", out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING("Plain ASCII text 123", out);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_strip_source_suffix_removes_trailing_source_name);
    RUN_TEST(test_strip_source_suffix_leaves_text_without_a_suffix_alone);
    RUN_TEST(test_decode_entities_handles_all_known_entities);
    RUN_TEST(test_decode_entities_passes_through_unrecognized_sequences);
    RUN_TEST(test_transliterate_converts_latin1_accents);
    RUN_TEST(test_transliterate_converts_smart_quotes_and_dashes);
    RUN_TEST(test_transliterate_passes_through_plain_ascii);
    return UNITY_END();
}
