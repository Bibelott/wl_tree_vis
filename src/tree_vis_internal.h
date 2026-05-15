#pragma once

#include <stdint.h>

#define COLOR_RED color_from_hex(0xFF0000)
#define COLOR_BLACK color_from_hex(0x000000)
#define COLOR_WHITE color_from_hex(0xFFFFFF)
#define COLOR_GREEN color_from_hex(0x00FF00)

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define abs(x) ((x) < 0 ? (-x) : (x))
#define sqr(x) ((x) * (x))
#define swap(a, b, T)                                                                              \
    do {                                                                                           \
        T tmp = a;                                                                                 \
        a = b;                                                                                     \
        b = tmp;                                                                                   \
    } while (0)
#define lerp(a, b, r) ((1 - (r)) * (a) + (r) * (b))
#define clamp(x, a, b) (((x) < (a)) ? (a) : (((x) > (b)) ? (b) : (x)))

