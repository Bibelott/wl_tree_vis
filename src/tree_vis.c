#include "tree_vis.h"
#include "dyn_array.h"
#include "llrb.c"
#include "tree_vis_internal.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

uint32_t in_bounds(PixelBuffer *buf, iVec2 point) {
    if (point.x < 0 || point.x >= buf->width || point.y < 0 || point.y >= buf->height)
        return 0;
    return 1;
}

Color blend(Color c1, Color c2, float opacity) {
    opacity = clamp(opacity, 0.0f, 1.0f);
    Color new_color = {0};
    new_color.r = lerp(c1.r, c2.r, opacity);
    new_color.g = lerp(c1.g, c2.g, opacity);
    new_color.b = lerp(c1.b, c2.b, opacity);
    return new_color;
}

void blend_in(PixelBuffer *buf, iVec2 p, Color color, float opacity) {
    uint32_t *raw = &buf->fer[p.y * buf->width + p.x];
    Color old_color = {.hex = *raw};
    Color new_color = blend(old_color, color, opacity);
    *raw = new_color.hex;
}

Line create_line(uint32_t point1, uint32_t point2, Color color) {
    Line line = {.start = point1, .end = point2, .color = color};
    return line;
}

// TODO: Determine whether this is an idiotic thing to do
void put_pixels_antialiased(PixelBuffer *buf, Vec2 position, Color color) {
    for (int32_t y = -1; y <= 1; y++) {
        for (int32_t x = -1; x <= 1; x++) {
            Vec2 p = {roundf(position.x + (float)x), roundf(position.y + (float)y)};
            iVec2 ip = {p.x, p.y};

            if (in_bounds(buf, ip)) {
                blend_in(buf, ip, color, 1.0f - vec_distance(position, p));
            }
        }
    }
}

// NOTE: Based on Wu's line generation algorithm (https://doi.org/10.1145%2F127719.122734)
void draw_line(PixelBuffer *buf, Array *points, Array *lines, uint32_t line_index) {
    Line line = arr_get(lines, line_index, Line);
    Vec2 point1 = arr_get(points, line.start, Vec2);
    Vec2 point2 = arr_get(points, line.end, Vec2);
    Color color = line.color;

    float y_diff = point1.y - point2.y;
    float x_diff = point1.x - point2.x;

    if (y_diff == 0.0f && x_diff == 0.0f)
        return;

    int32_t flipped = 0;

    if (abs(y_diff) > abs(x_diff)) {
        // flip the x and y coordinates of everything
        flipped = 1;

        swap(point1.x, point1.y, int32_t);
        swap(point2.x, point2.y, int32_t);
        swap(x_diff, y_diff, int32_t);
    }

    float slope = y_diff / x_diff;
    float offset = point1.y - slope * point1.x; // y = mx + b -> b = y - mx

    Vec2 start = point1.x < point2.x ? point1 : point2;
    Vec2 end = (start.x == point1.x && start.y == point1.y) ? point2 : point1;

    float x = max(start.x, 0.0f);
    float y = slope * x + offset; // y = mx + b

    for (; x <= min(end.x, (flipped ? buf->height : buf->width) - 1); x++) {
        Vec2 p;
        if (!flipped)
            p = vec_from_xy(x, y);
        else
            p = vec_from_xy(y, x);

        put_pixels_antialiased(buf, p, color);

        y += slope;
    }
}

Circle create_circle(uint32_t center, float radius, Color color) {
    Circle circle = {center, radius, color};
    return circle;
}

// NOTE: Based on Wu's midpoint circle generation (https://doi.org/10.1145%2F127719.122734), but
// filled in
void draw_circle(PixelBuffer *buf, Array *points, Array *circles, uint32_t circle_index) {
    Circle circle = arr_get(circles, circle_index, Circle);
    Vec2 center = arr_get(points, circle.center, Vec2);
    float radius = circle.radius;
    Color color = circle.color;

    float x = 0;
    float y = radius;

    while (x < y) {
        put_pixels_antialiased(buf, vec_from_xy(x + center.x, y + center.y), color);
        put_pixels_antialiased(buf, vec_from_xy(-x + center.x, y + center.y), color);
        put_pixels_antialiased(buf, vec_from_xy(-x + center.x, -y + center.y), color);
        put_pixels_antialiased(buf, vec_from_xy(x + center.x, -y + center.y), color);
        put_pixels_antialiased(buf, vec_from_xy(y + center.x, x + center.y), color);
        put_pixels_antialiased(buf, vec_from_xy(-y + center.x, x + center.y), color);
        put_pixels_antialiased(buf, vec_from_xy(-y + center.x, -x + center.y), color);
        put_pixels_antialiased(buf, vec_from_xy(y + center.x, -x + center.y), color);

        x += 1;
        y = sqrtf(sqr(radius) - sqr(x)); // x^2 + y^2 = r^2 -> y = (r^2 - x^2)^1/2
    }

    for (int32_t y = -radius + 1; y <= radius; y++) {
        int32_t x_bound = ceilf(sqrtf((float)sqr(radius) - (float)sqr(y)));
        for (int32_t x = -x_bound + 1; x <= x_bound; x++) {
            iVec2 position = {center.x + x, center.y + y};

            if (!in_bounds(buf, position))
                break;

            buf->fer[position.y * buf->width + position.x] = color.hex;
        }
    }
}

// TODO: Get rid of this entire thing. Implement a state machine thingy
void update_tree(PixelBuffer *buf, Array *points, Array *lines, Array *circles, Node *node,
                 float left_bound, float right_bound, float max_radius) {
    if (!node)
        return;

    Color color = llrb_is_node_red(node) ? COLOR_RED : COLOR_BLACK;

    float next_y = arr_get(points, node->point_index, Vec2).y + (float)buf->height / 10;
    float x_unit = (right_bound - left_bound) / 4;
    float left_x = left_bound + x_unit;
    float middle_x = left_x + x_unit;
    float right_x = middle_x + x_unit;

    float radius = min(1.75f * x_unit, max_radius);

    if (node->left) {
        if (node->left->point_index == -1) {
            Vec2 left = {left_x, next_y};
            node->left->point_index = arr_push(points, &left);

            Circle c = create_circle(node->left->point_index, radius, color);
            arr_push(circles, &c);

            Line l = create_line(node->point_index, node->left->point_index, COLOR_GREEN);
            arr_push(lines, &l);
        }

        update_tree(buf, points, lines, circles, node->left, left_bound,
                    min(middle_x, (float)buf->width), radius);
    }

    if (node->right) {
        if (node->right->point_index == -1) {
            Vec2 right = {right_x, next_y};
            node->right->point_index = arr_push(points, &right);

            Circle c = create_circle(node->right->point_index, radius, color);
            arr_push(circles, &c);

            Line l = create_line(node->point_index, node->right->point_index, COLOR_GREEN);
            arr_push(lines, &l);
        }

        update_tree(buf, points, lines, circles, node->right, max(0, middle_x), right_bound,
                    radius);
    }
}

void update_and_render(uint32_t *buffer, uint32_t width, uint32_t height, uint32_t time_delta) {
    static Node *root = NULL;
    static uint32_t node_timer = 0;
    static uint32_t counter = 1;
    static Array points = {0};
    static Array lines = {0};
    static Array circles = {0};

    // clear buffer
    memset(buffer, 0x2A, width * height * sizeof(uint32_t));

    if (points.capacity == 0)
        points = arr_create(sizeof(Vec2));

    if (lines.capacity == 0)
        lines = arr_create(sizeof(Line));

    if (circles.capacity == 0)
        circles = arr_create(sizeof(Circle));

    PixelBuffer buf = {0};
    buf.fer = buffer;
    buf.width = width;
    buf.height = height;

    node_timer += time_delta;

    if (node_timer > (2500 / (logf((float)counter) + 1))) {
        node_timer = 0;
        counter++;
        root = llrb_push(root, rand());

        if (root->point_index == -1) {
            Vec2 p = {(float)width / 2.0f, 50.0f};
            root->point_index = arr_push(&points, &p);
        }
    }

    update_tree(&buf, &points, &lines, &circles, root, (float)width / 20.0f,
                19.0f * (float)width / 20, 15);

    for (int i = 0; i < lines.length; i++) {
        draw_line(&buf, &points, &lines, i);
    }

    for (int i = 0; i < circles.length; i++) {
        draw_circle(&buf, &points, &circles, i);
    }
}
