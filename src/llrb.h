#pragma once

#include <stdint.h>

#define NODE_RED 0
#define NODE_BLACK 1

#define LLRB_LEFT 0
#define LLRB_RIGHT 1

typedef struct Node {
    int32_t value;
    uint32_t color;
    struct Node *parent;
    struct Node *left;
    struct Node *right;

    // TODO: Figure this shit out
    uint32_t point_index;
} Node;

typedef struct {
    int32_t value;
} LlrbPushArgs;

typedef struct {
    Node *node;
    int32_t value;
} LlrbInsertArgs;

typedef struct {
    Node *parent;
    int32_t value;
    uint32_t side;
} LlrbNewNodeArgs;

typedef struct {
    Node *node;
} LlrbUpArgs;

typedef struct {
    Node *node;
} LlrbRotateArgs;

typedef struct {
    Node *node;
} LlrbFlipColorsArgs;

typedef enum {
    LLRB_PUSH_ARGS,
    LLRB_INSERT_ARGS,
    LLRB_NEW_NODE_ARGS,
    LLRB_UP_ARGS,
    LLRB_ROTATE_ARGS,
    LLRB_FLIP_COLORS_ARGS,
} LlrbArgsType;

typedef struct {
    union {
        LlrbPushArgs push;
        LlrbInsertArgs insert;
        LlrbNewNodeArgs new_node;
        LlrbUpArgs up;
        LlrbRotateArgs rotate;
        LlrbFlipColorsArgs flip_colors;
    };
    LlrbArgsType type;
} LlrbArgs;

