#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include "action-queue.h"

// 环形队列
static action_item_t* g_QueueBuffer = NULL;
static size_t g_QueueCapacity = 0;
static volatile size_t g_QueueHead = 0;  // 生产者写入位置
static volatile size_t g_QueueTail = 0;  // 消费者读取位置
static volatile size_t g_QueueCount = 0;

// 同步原语
static CRITICAL_SECTION g_QueueLock;
static HANDLE g_QueueNotEmptyEvent = NULL;
static HANDLE g_WorkerThread = NULL;
static volatile bool g_WorkerRunning = false;
static volatile bool g_FlushRequested = false;

// 消费者工作线程
static DWORD WINAPI queue_worker_thread(LPVOID param) {
    (void)param;

    while (g_WorkerRunning) {
        // 等待非空
        WaitForSingleObject(g_QueueNotEmptyEvent, 100);

        EnterCriticalSection(&g_QueueLock);

        while (g_QueueCount > 0 && g_WorkerRunning) {
            action_item_t item = g_QueueBuffer[g_QueueTail];
            g_QueueTail = (g_QueueTail + 1) % g_QueueCapacity;
            g_QueueCount--;

            LeaveCriticalSection(&g_QueueLock);

            // 处理动作 — 调用 lib/autoxyq.c 中的对应函数
            // 由外部链接: autoxyq_key_down, autoxyq_mouse_move 等
            switch (item.type) {
                case ACTION_KEY_DOWN:
                    autoxyq_key_down(item.key_data.key);
                    break;
                case ACTION_KEY_UP:
                    autoxyq_key_up(item.key_data.key);
                    break;
                case ACTION_MOUSE_MOVE:
                    autoxyq_mouse_move_ex(item.move_data.dx, item.move_data.dy,
                        item.move_data.duration_ms,
                        (trajectory_type_t)item.move_data.traj_type);
                    break;
                case ACTION_MOUSE_BUTTON_DOWN:
                    autoxyq_mouse_button_down(item.button_data.button);
                    break;
                case ACTION_MOUSE_BUTTON_UP:
                    autoxyq_mouse_button_up(item.button_data.button);
                    break;
                case ACTION_MOUSE_SCROLL:
                    autoxyq_mouse_scroll(item.scroll_data.delta);
                    break;
                case ACTION_MOUSE_MOVE_TO:
                    autoxyq_mouse_move_to(item.move_to_data.x, item.move_to_data.y,
                        item.move_to_data.duration_ms,
                        (trajectory_type_t)item.move_to_data.traj_type);
                    break;
                case ACTION_FLUSH_MARKER:
                    g_FlushRequested = false;
                    break;
            }

            EnterCriticalSection(&g_QueueLock);
        }

        LeaveCriticalSection(&g_QueueLock);
    }

    return 0;
}

int action_queue_init(size_t capacity) {
    if (g_QueueBuffer != NULL) {
        return AUTOXYQ_OK; // 已初始化
    }

    g_QueueCapacity = (capacity > 0) ? capacity : 256;
    g_QueueBuffer = (action_item_t*)calloc(g_QueueCapacity, sizeof(action_item_t));
    if (g_QueueBuffer == NULL) {
        return AUTOXYQ_ERR_QUEUE_FULL;
    }

    InitializeCriticalSection(&g_QueueLock);

    g_QueueNotEmptyEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (g_QueueNotEmptyEvent == NULL) {
        free(g_QueueBuffer);
        g_QueueBuffer = NULL;
        return AUTOXYQ_ERR_QUEUE_FULL;
    }

    g_QueueHead = 0;
    g_QueueTail = 0;
    g_QueueCount = 0;
    g_WorkerRunning = true;

    g_WorkerThread = CreateThread(NULL, 0, queue_worker_thread, NULL, 0, NULL);
    if (g_WorkerThread == NULL) {
        CloseHandle(g_QueueNotEmptyEvent);
        free(g_QueueBuffer);
        g_QueueBuffer = NULL;
        return AUTOXYQ_ERR_QUEUE_FULL;
    }

    return AUTOXYQ_OK;
}

int action_queue_push(const action_item_t* item) {
    if (item == NULL || g_QueueBuffer == NULL) {
        return AUTOXYQ_ERR_INVALID_PARAM;
    }

    EnterCriticalSection(&g_QueueLock);

    if (g_QueueCount >= g_QueueCapacity) {
        LeaveCriticalSection(&g_QueueLock);
        return AUTOXYQ_ERR_QUEUE_FULL;
    }

    g_QueueBuffer[g_QueueHead] = *item;
    g_QueueHead = (g_QueueHead + 1) % g_QueueCapacity;
    g_QueueCount++;

    LeaveCriticalSection(&g_QueueLock);

    SetEvent(g_QueueNotEmptyEvent);
    return AUTOXYQ_OK;
}

void action_queue_flush(void) {
    if (g_QueueBuffer == NULL) return;

    action_item_t marker;
    marker.type = ACTION_FLUSH_MARKER;
    g_FlushRequested = true;
    action_queue_push(&marker);

    // 忙等待直到 flush marker 被处理
    while (g_FlushRequested) {
        Sleep(1);
    }
}

void action_queue_destroy(void) {
    g_WorkerRunning = false;

    if (g_WorkerThread != NULL) {
        SetEvent(g_QueueNotEmptyEvent);
        WaitForSingleObject(g_WorkerThread, 5000);
        CloseHandle(g_WorkerThread);
        g_WorkerThread = NULL;
    }

    if (g_QueueNotEmptyEvent != NULL) {
        CloseHandle(g_QueueNotEmptyEvent);
        g_QueueNotEmptyEvent = NULL;
    }

    DeleteCriticalSection(&g_QueueLock);

    free(g_QueueBuffer);
    g_QueueBuffer = NULL;
    g_QueueCapacity = 0;
}
