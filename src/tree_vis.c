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

// NOTE: Based on Wu's line generation algorithm (https://doi.org/10.1145%2F127719.122734)
void draw_line(PixelBuffer *buf, Array *points, Array *lines, uint32_t line_index, Vec2 scale) {
    Line line = arr_get(lines, line_index, Line);
    Vec2 point1 = arr_get(points, line.start, Vec2);
    Vec2 point2 = arr_get(points, line.end, Vec2);
    Color color = line.color;

    point1.x *= scale.x;
    point2.x *= scale.x;
    point1.x += scale.y;
    point2.x += scale.y;
    point1.y *= scale.x;
    point2.y *= scale.x;

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

    uint32_t x = (uint32_t)max(start.x + 0.5f, 0.5f);
    float y = slope * (float)x + offset; // y = mx + b

    for (; x <= min((uint32_t)(end.x + 0.5f), (flipped ? buf->height : buf->width) - 1); x++) {
        int32_t y_down = y;
        int32_t y_up = y_down + 1;
        float y_frac = y - (float)y_down;

        iVec2 p_down, p_up;

        if (!flipped) {
            p_down.x = x;
            p_down.y = y_down;
            p_up.x = x;
            p_up.y = y_up;

        } else {
            p_down.x = y_down;
            p_down.y = x;
            p_up.x = y_up;
            p_up.y = x;
        }

        blend_in(buf, p_down, color, 1 - y_frac);
        blend_in(buf, p_up, color, y_frac);

        y += slope;
    }
}

Circle create_circle(uint32_t center, float radius, Color color) {
    Circle circle = {center, radius, color};
    return circle;
}

void put_pixel_antialiased(PixelBuffer *buf, iVec2 p_down, Color color, float intensity,
                           iVec2 outside_dir) {
    iVec2 p_up = {p_down.x + outside_dir.x, p_down.y + outside_dir.y};

    if (in_bounds(buf, p_down))
        blend_in(buf, p_down, color, 1 - intensity);
    if (in_bounds(buf, p_up))
        blend_in(buf, p_up, color, intensity);
}

// NOTE: Based on Wu's midpoint circle generation (https://doi.org/10.1145%2F127719.122734), but
// filled in
void draw_circle(PixelBuffer *buf, Array *points, Array *circles, uint32_t circle_index,
                 Vec2 scale) {
    Circle circle = arr_get(circles, circle_index, Circle);
    Vec2 center = arr_get(points, circle.center, Vec2);
    center.x = (center.x * scale.x) + scale.y;
    center.y *= scale.x;

    float radius = circle.radius * scale.x;
    Color color = circle.color;

    int32_t x = 0;
    float y = radius;
    int32_t center_x = center.x + 0.5f;
    int32_t center_y = center.y + 0.5f;

    while (x < y) {
        int32_t y_down = y;
        float y_frac = y - (float)y_down;

        put_pixel_antialiased(buf, (iVec2){x + center_x, y_down + center_y}, color, y_frac,
                              (iVec2){0, 1});
        put_pixel_antialiased(buf, (iVec2){-x + center_x, y_down + center_y}, color, y_frac,
                              (iVec2){0, 1});
        put_pixel_antialiased(buf, (iVec2){-x + center_x, -y_down + center_y}, color, y_frac,
                              (iVec2){0, -1});
        put_pixel_antialiased(buf, (iVec2){x + center_x, -y_down + center_y}, color, y_frac,
                              (iVec2){0, -1});
        put_pixel_antialiased(buf, (iVec2){y_down + center_x, x + center_y}, color, y_frac,
                              (iVec2){1, 0});
        put_pixel_antialiased(buf, (iVec2){-y_down + center_x, x + center_y}, color, y_frac,
                              (iVec2){-1, 0});
        put_pixel_antialiased(buf, (iVec2){-y_down + center_x, -x + center_y}, color, y_frac,
                              (iVec2){-1, 0});
        put_pixel_antialiased(buf, (iVec2){y_down + center_x, -x + center_y}, color, y_frac,
                              (iVec2){1, 0});

        for (int32_t xi = center_x - x; xi <= center_x + x; xi++) {
            buf->fer[(center_y + y_down) * buf->width + xi] = color.hex;
            buf->fer[(center_y - y_down) * buf->width + xi] = color.hex;
        }

        for (int32_t xi = center_x - y_down; xi <= center_x + y_down; xi++) {
            buf->fer[(center_y + x) * buf->width + xi] = color.hex;
            buf->fer[(center_y - x) * buf->width + xi] = color.hex;
        }

        x += 1;
        y = sqrtf(sqr(radius) - sqr((float)x)); // x^2 + y^2 = r^2 -> y = (r^2 - x^2)^1/2
    }
}

// TODO: Get rid of this entire thing. Implement a state machine thingy
void update_tree(Array *points, Array *lines, Array *circles, Node *node, float left_bound,
                 float right_bound, float max_radius) {
    if (!node)
        return;

    Color color = llrb_is_node_red(node) ? COLOR_RED : COLOR_BLACK;

    float next_y = arr_get(points, node->point_index, Vec2).y + 0.2f;
    float x_unit = (right_bound - left_bound) / 4.0f;
    float left_x = left_bound + x_unit;
    float middle_x = left_x + x_unit;
    float right_x = middle_x + x_unit;

    // float radius = min(1.75f * x_unit, max_radius);
    float radius = max_radius;

    if (node->left) {
        if (node->left->point_index == -1) {
            Vec2 left = {left_x, next_y};
            node->left->point_index = arr_push(points, &left);

            Circle c = create_circle(node->left->point_index, radius, color);
            arr_push(circles, &c);

            Line l = create_line(node->point_index, node->left->point_index, COLOR_GREEN);
            arr_push(lines, &l);
        }

        update_tree(points, lines, circles, node->left, left_bound, middle_x, radius);
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

        update_tree(points, lines, circles, node->right, middle_x, right_bound, radius);
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
            Vec2 p = {0, 0.5f};
            root->point_index = arr_push(&points, &p);
        }
    }

    update_tree(&points, &lines, &circles, root, -1.0f, 1.0f, 0.1f);

    float min_x = 1;
    float max_x = -1;
    float max_y = -1;

    for (int i = 0; i < points.length; i++) {
        Vec2 point = arr_get(&points, i, Vec2);

        if (point.x < min_x)
            min_x = point.x;

        if (point.x > max_x)
            max_x = point.x;

        if (point.y > max_y)
            max_y = point.y;
    }

    max_x += 0.2f;
    min_x -= 0.2f;
    max_y += 0.2f;

    float scale_x = (float)buf.width / (2 * max(abs(max_x), abs(min_x)));
    float offset_x = (float)buf.width / 2;
    float scale_y = (float)buf.height / max_y;
    Vec2 scale = {min(scale_x, scale_y), offset_x};

    for (int i = 0; i < lines.length; i++) {
        draw_line(&buf, &points, &lines, i, scale);
    }

    for (int i = 0; i < circles.length; i++) {
        draw_circle(&buf, &points, &circles, i, scale);
    }
}
