#include "llrb.c"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define COLOR_RED 0xFF0000
#define COLOR_BLACK 0x000000
#define COLOR_WHITE 0xFFFFFF
#define COLOR_GREEN 0x00FF00

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

typedef struct {
    uint32_t *fer;
    uint32_t width;
    uint32_t height;
} PixelBuffer;

typedef struct {
    int32_t x;
    int32_t y;
} Point;

void draw_line(PixelBuffer *buf, Point point1, Point point2, uint32_t color) {
    int32_t y_diff = point1.y - point2.y;
    int32_t x_diff = point1.x - point2.x;

    if (y_diff == 0 && x_diff == 0)
        return;

    int32_t flipped = 0;

    if (abs(y_diff) > abs(x_diff)) {
        // flip the x and y coordinates of everything
        flipped = 1;

        swap(point1.x, point1.y, int32_t);
        swap(point2.x, point2.y, int32_t);
        swap(x_diff, y_diff, int32_t);
    }

    float slope = (float)y_diff / (float)x_diff;
    float offset = point1.y - slope * point1.x; // y = mx + b -> b = y - mx

    Point start = point1.x < point2.x ? point1 : point2;
    Point end = (start.x == point1.x && start.y == point1.y) ? point2 : point1;

    for (int32_t x = max(start.x, 0); x <= min(end.x, (flipped ? buf->height : buf->width) - 1);
         x++) {
        // y = mx + b
        int32_t y = slope * (float)x + offset;

        if (!flipped)
            buf->fer[y * buf->width + x] = color;
        else
            buf->fer[x * buf->width + y] = color;
    }
}

void draw_circle(PixelBuffer *buf, Point center, int32_t radius, uint32_t color) {
    // TODO: There's gotta be a better way
    for (int32_t y = max(center.y - radius, 0); y <= min(center.y + radius, buf->height - 1); y++) {

        for (int32_t x = max(center.x - radius, 0); x <= min(center.x + radius, buf->width - 1);
             x++) {

            if (sqr(x - center.x) + sqr(y - center.y) <= sqr(radius)) {
                buf->fer[y * buf->width + x] = color;
            }
        }
    }
}

void draw_tree(PixelBuffer *buf, Node *node, Point p, int32_t left_bound, int32_t right_bound,
               int32_t max_radius) {
    if (!node)
        return;

    uint32_t color = llrb_is_node_red(node) ? COLOR_RED : COLOR_BLACK;

    int32_t next_y = p.y + buf->height / 10;
    int32_t x_unit = (right_bound - left_bound) / 4;
    int32_t left_x = left_bound + x_unit;
    int32_t middle_x = left_x + x_unit;
    int32_t right_x = middle_x + x_unit;

    Point left = {left_x, next_y};
    Point right = {right_x, next_y};

    int32_t radius = (int32_t)min(1.75 * (float)x_unit, (float)max_radius);

    if (node->left) {
        draw_line(buf, p, left, COLOR_GREEN);
        draw_tree(buf, node->left, left, left_bound, min(middle_x, buf->width), radius);
    }

    if (node->right) {
        draw_line(buf, p, right, COLOR_GREEN);
        draw_tree(buf, node->right, right, max(0, middle_x), right_bound, radius);
    }

    draw_circle(buf, p, radius, color);
}

void update_and_render(uint32_t *buffer, uint32_t width, uint32_t height, uint32_t time_delta) {
    static Node *root = NULL;
    static uint32_t node_timer = 0;
    static uint32_t counter = 1;

    // clear buffer
    memset(buffer, 0x2A, width * height * sizeof(uint32_t));

    PixelBuffer buf = {0};
    buf.fer = buffer;
    buf.width = width;
    buf.height = height;

    Point p = {width / 2, 50};

    node_timer += time_delta;

    if (node_timer > (2500 / (logf((float)counter) + 1))) {
        node_timer = 0;
        counter++;
        root = llrb_push(root, rand());
    }

    draw_tree(&buf, root, p, width / 20, 19 * width / 20, 15);
}
