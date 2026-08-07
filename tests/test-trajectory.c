#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "trajectory.h"
#include "autoxyq-error.h"

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { printf("  TEST: %s ... ", name); tests_run++; } while(0)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); } while(0)

// 外部变量 (trajectory.c 需要)
uint8_t g_JitterAmplitude = 0;  // 测试时关闭抖动
uint32_t g_RandomSeed = 12345;

void test_trajectory_linear(void) {
    TEST("linear trajectory generates path");
    int16_t* path = NULL;
    uint32_t count = 0;
    int ret = trajectory_generate_path(0, 0, 100, 0, 200,
        TRAJECTORY_LINEAR, &path, &count);
    if (ret != AUTOXYQ_OK) {
        FAIL("trajectory_generate_path failed");
        return;
    }
    if (count < 2) {
        FAIL("path too short");
        free(path);
        return;
    }
    // 终点精确到达
    if (path[(count - 1) * 2] != 100 || path[(count - 1) * 2 + 1] != 0) {
        FAIL("endpoint mismatch");
        free(path);
        return;
    }
    free(path);
    PASS();
}

void test_trajectory_bezier3(void) {
    TEST("bezier3 trajectory generates path");
    int16_t* path = NULL;
    uint32_t count = 0;
    int ret = trajectory_generate_path(0, 0, 200, -100, 500,
        TRAJECTORY_BEZIER3, &path, &count);
    if (ret != AUTOXYQ_OK) {
        FAIL("trajectory_generate_path failed");
        return;
    }
    if (count < 2) {
        FAIL("path too short");
        free(path);
        return;
    }
    // 终点精确到达
    if (path[(count - 1) * 2] != 200 || path[(count - 1) * 2 + 1] != -100) {
        FAIL("endpoint mismatch");
        free(path);
        return;
    }
    free(path);
    PASS();
}

void test_trajectory_invalid_params(void) {
    TEST("invalid parameters return error");
    int16_t* path = NULL;
    uint32_t count = 0;

    int ret = trajectory_generate_path(0, 0, 0, 0, 0,
        TRAJECTORY_LINEAR, &path, &count);
    if (ret != AUTOXYQ_ERR_INVALID_PARAM) {
        FAIL("should return INVALID_PARAM for zero duration");
        return;
    }

    ret = trajectory_generate_path(0, 0, 0, 0, 100,
        TRAJECTORY_LINEAR, NULL, &count);
    if (ret != AUTOXYQ_ERR_INVALID_PARAM) {
        FAIL("should return INVALID_PARAM for NULL output");
        return;
    }

    PASS();
}

int main(void) {
    srand(12345);
    printf("=== trajectory engine tests ===\n");
    test_trajectory_linear();
    test_trajectory_bezier3();
    test_trajectory_invalid_params();
    printf("Results: %d/%d passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
