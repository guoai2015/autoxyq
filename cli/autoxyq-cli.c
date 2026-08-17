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
    printf("  autoxyq-cli hotkey <modifier...> <key> [duration_ms]\n");
    printf("  autoxyq-cli move <dx> <dy> [duration_ms] [trajectory:0=linear,1=bezier2,2=bezier3]\n");
    printf("  autoxyq-cli moveto <x> <y> <duration_ms> [trajectory]\n");
    printf("  autoxyq-cli button <1-5> <down|up>\n");
    printf("  autoxyq-cli scroll <delta>\n");
    printf("  autoxyq-cli delay <min_ms> <max_ms>\n");
    printf("  autoxyq-cli reset\n");
}

// 解析按键参数: 支持修饰键友好名 (不区分大小写) 和十六进制/十进制数字
// 返回 USB HID Usage ID (0-255), 无法识别返回 -1
static int parse_key_id(const char* s) {
    if (_stricmp(s, "ctrl") == 0 || _stricmp(s, "control") == 0 ||
        _stricmp(s, "lctrl") == 0 || _stricmp(s, "lcontrol") == 0) return 0xE0;
    if (_stricmp(s, "shift") == 0 || _stricmp(s, "lshift") == 0) return 0xE1;
    if (_stricmp(s, "alt") == 0 || _stricmp(s, "lalt") == 0) return 0xE2;
    if (_stricmp(s, "win") == 0 || _stricmp(s, "lwin") == 0 ||
        _stricmp(s, "gui") == 0 || _stricmp(s, "lgui") == 0) return 0xE3;
    if (_stricmp(s, "rctrl") == 0 || _stricmp(s, "rcontrol") == 0) return 0xE4;
    if (_stricmp(s, "rshift") == 0) return 0xE5;
    if (_stricmp(s, "ralt") == 0) return 0xE6;
    if (_stricmp(s, "rwin") == 0 || _stricmp(s, "rgui") == 0) return 0xE7;

    char* end = NULL;
    long v = strtol(s, &end, 0);
    if (s != end && *end == '\0' && v >= 0 && v <= 0xFF) {
        return (int)v;
    }
    return -1;
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
        int key = (int)strtol(argv[2], NULL, 0);
        int duration = (argc >= 4) ? atoi(argv[3]) : 0;
        if (duration > 0) {
            ret = autoxyq_key_press((uint8_t)key, (uint32_t)duration);
        } else {
            ret = autoxyq_key_down((uint8_t)key);
            autoxyq_flush();
            ret = autoxyq_key_up((uint8_t)key);
        }
        printf("Key: 0x%02X duration=%dms -> %s\n", key, duration, autoxyq_strerror(ret));

    } else if (strcmp(cmd, "hotkey") == 0 && argc >= 4) {
        // hotkey <修饰键...> <主键> [按住时长_ms]
        // 最后一个参数若为纯十进制数字, 视为按住时长
        int last = argc - 1;
        int duration = 0;
        if (argc - 2 >= 3) {
            char* end = NULL;
            long v = strtol(argv[last], &end, 10);
            if (argv[last][0] != '\0' && *end == '\0') {
                duration = (int)v;
                last--;
            }
        }

        int main_key = parse_key_id(argv[last]);
        if (main_key < 0) {
            fprintf(stderr, "Hotkey: unknown key '%s'\n", argv[last]);
            ret = AUTOXYQ_ERR_INVALID_PARAM;
        } else {
            // 按下所有修饰键
            ret = AUTOXYQ_OK;
            for (int i = 2; i < last && ret == AUTOXYQ_OK; i++) {
                int mod = parse_key_id(argv[i]);
                if (mod < 0xE0 || mod > 0xE7) {
                    fprintf(stderr, "Hotkey: '%s' is not a modifier key\n", argv[i]);
                    ret = AUTOXYQ_ERR_INVALID_PARAM;
                    break;
                }
                ret = autoxyq_key_down((uint8_t)mod);
            }
            // 主键按下 + 延时 + 弹起
            if (ret == AUTOXYQ_OK) {
                ret = autoxyq_key_press((uint8_t)main_key, (uint32_t)duration);
            }
            // 逆序弹起修饰键 (即便中途失败也清理已按下的)
            for (int i = last - 1; i >= 2; i--) {
                int mod = parse_key_id(argv[i]);
                if (mod >= 0xE0 && mod <= 0xE7) {
                    autoxyq_key_up((uint8_t)mod);
                }
            }
            printf("Hotkey: duration=%dms -> %s\n", duration, autoxyq_strerror(ret));
        }

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
