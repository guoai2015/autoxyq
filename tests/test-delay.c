#include <stdio.h>
#include <assert.h>
#include <windows.h>
#include "delay.h"

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { \
    printf("  TEST: %s ... ", name); \
    tests_run++; \
} while(0)

#define PASS() do { \
    printf("PASS\n"); \
    tests_passed++; \
} while(0)

#define FAIL(msg) do { \
    printf("FAIL: %s\n", msg); \
} while(0)

void test_delay_range_default(void) {
    TEST("default delay range returns valid values");
    for (int i = 0; i < 100; i++) {
        uint32_t d = delay_random_ms();
        if (d < 10 || d > 50) {
            FAIL("delay out of default range [10, 50]");
            printf("     got %u\n", d);
            return;
        }
    }
    PASS();
}

void test_delay_range_custom(void) {
    TEST("custom delay range [100, 200]");
    delay_set_range(100, 200);
    for (int i = 0; i < 100; i++) {
        uint32_t d = delay_random_ms();
        if (d < 100 || d > 200) {
            FAIL("delay out of custom range");
            printf("     got %u\n", d);
            return;
        }
    }
    PASS();
}

void test_delay_min_equals_max(void) {
    TEST("min equals max returns exact value");
    delay_set_range(42, 42);
    for (int i = 0; i < 20; i++) {
        uint32_t d = delay_random_ms();
        if (d != 42) {
            FAIL("delay should be exactly 42");
            printf("     got %u\n", d);
            return;
        }
    }
    PASS();
}

void test_delay_sleep_accuracy(void) {
    TEST("sleep_us accuracy within tolerance");
    DWORD before = GetTickCount();
    delay_sleep_us(100000); // 100ms
    DWORD after = GetTickCount();
    DWORD elapsed = after - before;
    if (elapsed < 90 || elapsed > 150) {
        FAIL("sleep accuracy out of tolerance");
        printf("     expected ~100ms, got %lums\n", elapsed);
        return;
    }
    PASS();
}

int main(void) {
    printf("=== delay engine tests ===\n");
    test_delay_range_default();
    test_delay_range_custom();
    test_delay_min_equals_max();
    test_delay_sleep_accuracy();
    printf("Results: %d/%d passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
