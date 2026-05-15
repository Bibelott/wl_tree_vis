#pragma once

#include <stdint.h>

void update_and_render(uint32_t *buffer, uint32_t width, uint32_t height,
            uint32_t time_delta);

typedef struct {
    uint32_t *fer;
    uint32_t width;
    uint32_t height;
} PixelBuffer;

// TODO: Switch over to virtual space with floats
typedef struct {
    int32_t x;
    int32_t y;
} Vec2;

typedef union {
    struct {
        uint8_t x;
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };
    uint32_t hex;
} Color;

typedef struct {
    uint32_t start;
    uint32_t end;
    Color color;
} Line;

typedef struct {
    uint32_t center;
    uint32_t radius;
    Color color;
} Circle;

