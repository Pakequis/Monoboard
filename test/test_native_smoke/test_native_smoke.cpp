#include <unity.h>

void setUp() {}
void tearDown() {}

void test_smoke_arithmetic()
{
    TEST_ASSERT_EQUAL(4, 2 + 2);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_smoke_arithmetic);
    return UNITY_END();
}
