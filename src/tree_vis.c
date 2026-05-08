#include "dyn_array.c"
#include "llrb.c"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

typedef struct {
    uint32_t *fer;
    uint32_t width;
    uint32_t height;
} PixelBuffer;

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
    Vec2 *start;
    Vec2 *end;
    Color color;
} Line;

typedef struct {
    Vec2 *center;
    uint32_t radius;
    Color color;
} Circle;

Color color_from_hex(uint32_t hex) {
    Color color = {.hex = hex};
    return color;
}

Color color_from_rgb(uint8_t r, uint8_t g, uint8_t b) {
    Color color = {.r = r, .g = g, .b = b};
    return color;
}

Vec2 vec_from_xy(int32_t x, int32_t y) {
    Vec2 v = {.x = x, .y = y};
    return v;
}

uint32_t in_bounds(PixelBuffer *buf, Vec2 point) {
    if (point.x < 0 || point.x >= buf->width || point.y < 0 || point.y >= buf->height)
        return 0;
    return 1;
}

Color blend(Color c1, Color c2, float opacity) {
    Color new_color = {0};
    new_color.r = lerp(c1.r, c2.r, opacity);
    new_color.g = lerp(c1.g, c2.g, opacity);
    new_color.b = lerp(c1.b, c2.b, opacity);
    return new_color;
}

void blend_in(PixelBuffer *buf, Vec2 p, Color color, float opacity) {
    uint32_t *raw = &buf->fer[p.y * buf->width + p.x];
    Color old_color = {.hex = *raw};
    Color new_color = blend(old_color, color, opacity);
    *raw = new_color.hex;
}

Line create_line(Vec2 *point1, Vec2 *point2, Color color) {
    Line line = {.start = point1, .end = point2, .color = color};
    return line;
}

// NOTE: Based on Wu's line generation algorithm (https://doi.org/10.1145%2F127719.122734)
void draw_line(PixelBuffer *buf, Line *line) {
    Vec2 point1 = *line->start;
    Vec2 point2 = *line->end;
    Color color = line->color;

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

    Vec2 start = point1.x < point2.x ? point1 : point2;
    Vec2 end = (start.x == point1.x && start.y == point1.y) ? point2 : point1;

    int32_t x = max(start.x, 0);
    float y = slope * (float)x + offset; // y = mx + b

    for (; x <= min(end.x, (flipped ? buf->height : buf->width) - 1); x++) {
        int32_t y_down = y;
        int32_t y_up = y + 1;
        float y_frac = y - (float)y_down;
        Vec2 p_down, p_up;

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

void put_pixel_antialiased(PixelBuffer *buf, Vec2 position, Color color, float intensity,
                           Vec2 outside_dir) {
    Vec2 p_down = {position.x, position.y};
    Vec2 p_up = {position.x + outside_dir.x, position.y + outside_dir.y};

    if (in_bounds(buf, p_down))
        blend_in(buf, p_down, color, 1 - intensity);
    if (in_bounds(buf, p_up))
        blend_in(buf, p_up, color, intensity);
}

Circle create_circle(Vec2 *center, uint32_t radius, Color color) {
    Circle circle = {center, radius, color};
    return circle;
}

// NOTE: Based on Wu's midpoint circle generation (https://doi.org/10.1145%2F127719.122734), but
// filled in
void draw_circle(PixelBuffer *buf, Circle *circle) {
    Vec2 center = *circle->center;
    int32_t radius = circle->radius;
    Color color = circle->color;

    int32_t x = 0;
    float y = radius;

    while (x < y) {
        int32_t y_down = y;
        float y_frac = y - (float)y_down;

        put_pixel_antialiased(buf, vec_from_xy(x + center.x, y_down + center.y), color, y_frac,
                              vec_from_xy(0, 1));
        put_pixel_antialiased(buf, vec_from_xy(-x + center.x, y_down + center.y), color, y_frac,
                              vec_from_xy(0, 1));
        put_pixel_antialiased(buf, vec_from_xy(-x + center.x, -y_down + center.y), color, y_frac,
                              vec_from_xy(0, -1));
        put_pixel_antialiased(buf, vec_from_xy(x + center.x, -y_down + center.y), color, y_frac,
                              vec_from_xy(0, -1));
        put_pixel_antialiased(buf, vec_from_xy(y_down + center.x, x + center.y), color, y_frac,
                              vec_from_xy(1, 0));
        put_pixel_antialiased(buf, vec_from_xy(-y_down + center.x, x + center.y), color, y_frac,
                              vec_from_xy(-1, 0));
        put_pixel_antialiased(buf, vec_from_xy(-y_down + center.x, -x + center.y), color, y_frac,
                              vec_from_xy(-1, 0));
        put_pixel_antialiased(buf, vec_from_xy(y_down + center.x, -x + center.y), color, y_frac,
                              vec_from_xy(1, 0));

        x += 1;
        y = sqrtf((float)sqr(radius) - (float)sqr(x)); // x^2 + y^2 = r^2 -> y = (r^2 - x^2)^1/2
    }

    for (int32_t y = -radius + 1; y < radius; y++) {
        int32_t x_bound = ceilf(sqrtf((float)sqr(radius) - (float)sqr(y)));
        for (int32_t x = -x_bound + 1; x < x_bound; x++) {
            Vec2 position = {center.x + x, center.y + y};

            if (!in_bounds(buf, position))
                break;

            buf->fer[position.y * buf->width + position.x] = color.hex;
        }
    }
}

// TODO: Associate circles with nodes
void update_tree(PixelBuffer *buf, Array *points, Array *lines, Array *circles, Node *node,
                 Vec2 *position, int32_t left_bound, int32_t right_bound, int32_t max_radius) {
    if (!node)
        return;

    Color color = llrb_is_node_red(node) ? COLOR_RED : COLOR_BLACK;

    int32_t next_y = position->y + buf->height / 10;
    int32_t x_unit = (right_bound - left_bound) / 4;
    int32_t left_x = left_bound + x_unit;
    int32_t middle_x = left_x + x_unit;
    int32_t right_x = middle_x + x_unit;

    Vec2 left = {left_x, next_y};
    Vec2 right = {right_x, next_y};

    Vec2 *leftp = arr_push_unique(points, left);
    Vec2 *rightp = arr_push_unique(points, right);

    int32_t radius = (int32_t)min(1.75 * (float)x_unit, (float)max_radius);

    if (node->left) {
        Line l = create_line(position, leftp, COLOR_GREEN);
        arr_push_unique(lines, l);
        update_tree(buf, points, lines, circles, node->left, leftp, left_bound,
                    min(middle_x, buf->width), radius);
    }

    if (node->right) {
        Line l = create_line(position, rightp, COLOR_GREEN);
        arr_push_unique(lines, l);
        update_tree(buf, points, lines, circles, node->right, rightp, max(0, middle_x), right_bound,
                    radius);
    }

    Circle c = create_circle(position, radius, color);
    arr_push_unique(circles, c);
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

    PixelBuffer buf = {0};
    buf.fer = buffer;
    buf.width = width;
    buf.height = height;

    Vec2 p = {width / 2, 50};
    Vec2 *pp = arr_push_unique(&points, p);

    node_timer += time_delta;

    if (node_timer > (2500 / (logf((float)counter) + 1))) {
        node_timer = 0;
        counter++;
        root = llrb_push(root, rand());
    }

    update_tree(&buf, &points, &lines, &circles, root, pp, width / 20, 19 * width / 20, 15);

    for (int i = 0; i < arr_len(&lines, Line); i++) {
        Line *l = &arr_get(&lines, i, Line);
        draw_line(&buf, l);
    }

    for (int i = 0; i < arr_len(&circles, Circle); i++) {
        Circle *c = &arr_get(&circles, i, Circle);
        draw_circle(&buf, c);
    }
}
