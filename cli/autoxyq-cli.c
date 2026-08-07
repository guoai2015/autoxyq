#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "autoxyq.h"

// 简易 CLI — 接收命令参数
// 用法:
//   autoxyq-cli key <usb_usage_id> [duration_ms]
//   autoxyq-cli move <dx> <dy> [duration_ms] [trajectory]
//   autoxyq-cli moveto <x> <y> <duration_ms> [trajectory]
//   autoxyq-cli button <1-5> <down|up>
//   autoxyq-cli scroll <delta>
//   autoxyq-cli delay <min_ms> <max_ms>
//   autoxyq-cli reset

static void print_usage(void) {
    printf("autoxyq-cli - Virtual HID Keyboard & Mouse CLI\n");
    printf("Usage:\n");
    printf("  autoxyq-cli key <usb_usage_id> [duration_ms]\n");
    printf("  autoxyq-cli move <dx> <dy> [duration_ms] [trajectory:0=linear,1=bezier2,2=bezier3]\n");
    printf("  autoxyq-cli moveto <x> <y> <duration_ms> [trajectory]\n");
    printf("  autoxyq-cli button <1-5> <down|up>\n");
    printf("  autoxyq-cli scroll <delta>\n");
    printf("  autoxyq-cli delay <min_ms> <max_ms>\n");
    printf("  autoxyq-cli reset\n");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    int ret = autoxyq_init();
    if (ret != AUTOXYQ_OK) {
        fprintf(stderr, "Error: %s\n", autoxyq_strerror(ret));
        return ret;
    }

    const char* cmd = argv[1];

    if (strcmp(cmd, "key") == 0 && argc >= 3) {
        int key = atoi(argv[2]);
        int duration = (argc >= 4) ? atoi(argv[3]) : 0;
        if (duration > 0) {
            ret = autoxyq_key_press((uint8_t)key, (uint32_t)duration);
        } else {
            ret = autoxyq_key_down((uint8_t)key);
            autoxyq_flush();
            ret = autoxyq_key_up((uint8_t)key);
        }
        printf("Key: 0x%02X duration=%dms -> %s\n", key, duration, autoxyq_strerror(ret));

    } else if (strcmp(cmd, "move") == 0 && argc >= 4) {
        int dx = atoi(argv[2]);
        int dy = atoi(argv[3]);
        int duration = (argc >= 5) ? atoi(argv[4]) : 0;
        int traj = (argc >= 6) ? atoi(argv[5]) : TRAJECTORY_BEZIER3;
        ret = autoxyq_mouse_move_ex((int16_t)dx, (int16_t)dy,
            (uint32_t)duration, (trajectory_type_t)traj);
        printf("Move: dx=%d dy=%d duration=%dms -> %s\n", dx, dy, duration, autoxyq_strerror(ret));

    } else if (strcmp(cmd, "moveto") == 0 && argc >= 5) {
        int x = atoi(argv[2]);
        int y = atoi(argv[3]);
        int duration = atoi(argv[4]);
        int traj = (argc >= 6) ? atoi(argv[5]) : TRAJECTORY_BEZIER3;
        ret = autoxyq_mouse_move_to(x, y, (uint32_t)duration, (trajectory_type_t)traj);
        printf("MoveTo: x=%d y=%d duration=%dms -> %s\n", x, y, duration, autoxyq_strerror(ret));

    } else if (strcmp(cmd, "button") == 0 && argc >= 4) {
        int button = atoi(argv[2]);
        if (strcmp(argv[3], "down") == 0) {
            ret = autoxyq_mouse_button_down((uint8_t)button);
        } else {
            ret = autoxyq_mouse_button_up((uint8_t)button);
        }
        printf("Button %d %s -> %s\n", button, argv[3], autoxyq_strerror(ret));

    } else if (strcmp(cmd, "scroll") == 0 && argc >= 3) {
        int delta = atoi(argv[2]);
        ret = autoxyq_mouse_scroll((int8_t)delta);
        printf("Scroll: %d -> %s\n", delta, autoxyq_strerror(ret));

    } else if (strcmp(cmd, "delay") == 0 && argc >= 4) {
        int min_ms = atoi(argv[2]);
        int max_ms = atoi(argv[3]);
        autoxyq_set_delay_range((uint32_t)min_ms, (uint32_t)max_ms);
        printf("Delay range: [%d, %d] ms\n", min_ms, max_ms);

    } else if (strcmp(cmd, "reset") == 0) {
        ret = autoxyq_mouse_button_up(1);
        ret = autoxyq_mouse_button_up(2);
        ret = autoxyq_mouse_button_up(3);
        ret = autoxyq_mouse_button_up(4);
        ret = autoxyq_mouse_button_up(5);
        printf("Reset -> %s\n", autoxyq_strerror(ret));

    } else {
        print_usage();
    }

    autoxyq_flush();
    autoxyq_shutdown();
    return ret;
}
