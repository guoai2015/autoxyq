#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "action-queue.h"

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { printf("  TEST: %s ... ", name); tests_run++; } while(0)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); } while(0)

void test_queue_init(void) {
    TEST("queue initialization");
    int ret = action_queue_init(64);
    if (ret != AUTOXYQ_OK) {
        FAIL("init failed");
        return;
    }
    action_queue_destroy();
    PASS();
}

void test_queue_push(void) {
    TEST("queue push does not fail");
    action_queue_init(8);
    action_item_t item;
    item.type = ACTION_KEY_DOWN;
    item.key_data.key = 0x04; // 'A'
    item.key_data.duration_ms = 0;

    int ret = action_queue_push(&item);
    if (ret != AUTOXYQ_OK) {
        FAIL("push failed");
        action_queue_destroy();
        return;
    }

    // 给工作线程一点时间处理
    Sleep(100);
    action_queue_destroy();
    PASS();
}

void test_queue_full(void) {
    TEST("queue full returns error");
    action_queue_init(2);

    action_item_t item;
    item.type = ACTION_KEY_DOWN;
    item.key_data.key = 0x04;
    item.key_data.duration_ms = 0;

    action_queue_push(&item);
    action_queue_push(&item);
    int ret = action_queue_push(&item);

    if (ret != AUTOXYQ_ERR_QUEUE_FULL) {
        FAIL("should return QUEUE_FULL");
        action_queue_destroy();
        return;
    }

    Sleep(200);
    action_queue_destroy();
    PASS();
}

int main(void) {
    printf("=== action queue tests ===\n");
    test_queue_init();
    test_queue_push();
    test_queue_full();
    printf("Results: %d/%d passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
