#pragma once

#include <stdint.h>
#include <math.h>

void update_and_render(uint32_t *buffer, uint32_t width, uint32_t height,
            uint32_t time_delta);

typedef struct {
    uint32_t *fer;
    uint32_t width;
    uint32_t height;
} PixelBuffer;

typedef struct {
    float x;
    float y;
} Vec2;

Vec2 vec_from_xy(float x, float y) {
    Vec2 v = {.x = x, .y = y};
    return v;
}

Vec2 vec_subtract(Vec2 a, Vec2 b) {
    Vec2 result = {a.x - b.x, a.y - b.y};
    return result;
}

float vec_length(Vec2 v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}

float vec_distance(Vec2 a, Vec2 b) {
    return vec_length(vec_subtract(a, b));
}

typedef struct {
    int32_t x;
    int32_t y;
} iVec2;

iVec2 ivec_from_xy(int32_t x, int32_t y) {
    iVec2 v = {.x = x, .y = y};
    return v;
}

typedef union {
    struct {
        uint8_t b;
        uint8_t g;
        uint8_t r;
        uint8_t x;
    };
    uint32_t hex;
} Color;

Color color_from_hex(uint32_t hex) {
    Color color = {.hex = hex};
    return color;
}

Color color_from_rgb(uint8_t r, uint8_t g, uint8_t b) {
    Color color = {.r = r, .g = g, .b = b};
    return color;
}

typedef struct {
    uint32_t start;
    uint32_t end;
    Color color;
} Line;

typedef struct {
    uint32_t center;
    float radius;
    Color color;
} Circle;

