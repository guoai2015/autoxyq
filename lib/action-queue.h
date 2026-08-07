#ifndef ACTION_QUEUE_H
#define ACTION_QUEUE_H

#include <stdint.h>
#include <stdbool.h>
#include "autoxyq-error.h"

// 动作类型
typedef enum {
    ACTION_KEY_DOWN,
    ACTION_KEY_UP,
    ACTION_MOUSE_MOVE,
    ACTION_MOUSE_BUTTON_DOWN,
    ACTION_MOUSE_BUTTON_UP,
    ACTION_MOUSE_SCROLL,
    ACTION_MOUSE_MOVE_TO,
    ACTION_FLUSH_MARKER,
} action_type_t;

// 动作队列项
typedef struct {
    action_type_t type;
    union {
        struct { uint8_t key; uint32_t duration_ms; } key_data;
        struct { int16_t dx; int16_t dy; uint32_t duration_ms; int traj_type; } move_data;
        struct { int x; int y; uint32_t duration_ms; int traj_type; } move_to_data;
        struct { uint8_t button; } button_data;
        struct { int8_t delta; } scroll_data;
    };
} action_item_t;

// 初始化动作队列
int action_queue_init(size_t capacity);

// 入队 (生产者)
int action_queue_push(const action_item_t* item);

// 阻塞等待队列清空
void action_queue_flush(void);

// 销毁队列
void action_queue_destroy(void);

#endif // ACTION_QUEUE_H
