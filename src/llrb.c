#include "llrb.h"
#include "dyn_array.h"
#include "tree_machine.h"
#include "tree_vis.h"
#include "tree_vis_internal.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int32_t llrb_is_node_red(Node *node) {
    if (node && node->color == NODE_RED)
        return 1;
    else
        return 0;
}

#if 0
Node *llrb_new_node(int32_t value) {
    Node *node = malloc(sizeof(Node));
    memset(node, 0, sizeof(Node));

    node->value = value;
    node->point_index = -1;
    // node->color = NODE_RED; // implicit since NODE_RED == 0

    return node;
}

void llrb_flip_colors(Node *node) {
    node->color = !node->color;
    node->left->color = !node->left->color;
    node->right->color = !node->right->color;
}

Node *llrb_rotate_left(Node *node) {
    Node *x = node->right;
    node->right = x->left;
    x->left = node;
    x->color = node->color;
    node->color = NODE_RED;
    return x;
}

Node *llrb_rotate_right(Node *node) {
    Node *x = node->left;
    node->left = x->right;
    x->right = node;
    x->color = node->color;
    node->color = NODE_RED;
    return x;
}

Node *llrb_insert(Node *node, int32_t value) {
    if (!node)
        return llrb_new_node(value);

    if (value == node->value)
        goto up;
    else if (value < node->value)
        node->left = llrb_insert(node->left, value);
    else
        node->right = llrb_insert(node->right, value);

up:
    if (llrb_is_node_red(node->right) && !llrb_is_node_red(node->left)) {
        node = llrb_rotate_left(node);
    }
    if (llrb_is_node_red(node->left) && llrb_is_node_red(node->left->left)) {
        node = llrb_rotate_right(node);
    }

    if (llrb_is_node_red(node->left) && llrb_is_node_red(node->right)) {
        llrb_flip_colors(node);
    }

    return node;
}

Node *llrb_push(Node *root, int32_t value) {
    Node *node = llrb_insert(root, value);
    node->color = NODE_BLACK;

    return node;
}
#else

bool llrb_flip_colors(TreeState *state, uint32_t args_index) {
    LlrbFlipColorsArgs args =
        (&arr_get(&state->args, args_index, LlrbArgs))->flip_colors;

    Node *node = args.node;

    node->color = !node->color;
    node->left->color = !node->left->color;
    node->right->color = !node->right->color;

    return true;
}

bool llrb_rotate_right(TreeState *state, uint32_t args_index) {
    LlrbRotateArgs args =
        (&arr_get(&state->args, args_index, LlrbArgs))->rotate;

    Node *node = args.node;

    Node *x = node->left;
    node->left = x->right;
    x->right = node;
    x->color = node->color;
    node->color = NODE_RED;
    // return x;
    node = x;

    if (llrb_is_node_red(node->left) && llrb_is_node_red(node->right)) {
        // llrb_flip_colors(node);
        uint32_t flip_args =
            arr_push(&state->args, &(LlrbArgs){.flip_colors = {.node = node},
                                               .type = LLRB_FLIP_COLORS_ARGS});

        Operation flip_op = {.action = llrb_flip_colors,
                             .args_index = flip_args,
                             .timer = state->time + 1000};
        arr_push(&state->operations, &flip_op);
    }

    return true;
}

bool llrb_rotate_left(TreeState *state, uint32_t args_index) {
    LlrbRotateArgs args =
        (&arr_get(&state->args, args_index, LlrbArgs))->rotate;

    Node *node = args.node;

    Node *x = node->right;
    node->right = x->left;
    x->left = node;
    x->color = node->color;
    node->color = NODE_RED;
    // return x;
    node = x;

    if (llrb_is_node_red(node->left) && llrb_is_node_red(node->left->left)) {
        // node = llrb_rotate_right(node);
        uint32_t rot_args =
            arr_push(&state->args, &(LlrbArgs){.rotate = {.node = node->parent},
                                               .type = LLRB_ROTATE_ARGS});
        Operation rot_op = {.action = llrb_rotate_right,
                            .args_index = rot_args,
                            .timer = state->time + 1000};
        arr_push(&state->operations, &rot_op);
    }
    if (llrb_is_node_red(node->left) && llrb_is_node_red(node->right)) {
        // llrb_flip_colors(node);
        uint32_t flip_args =
            arr_push(&state->args, &(LlrbArgs){.flip_colors = {.node = node},
                                               .type = LLRB_FLIP_COLORS_ARGS});

        Operation flip_op = {.action = llrb_flip_colors,
                             .args_index = flip_args,
                             .timer = state->time + 1000};
        arr_push(&state->operations, &flip_op);
    }

    return true;
}

bool llrb_up(TreeState *state, uint32_t args_index) {
    LlrbUpArgs args = (&arr_get(&state->args, args_index, LlrbArgs))->up;

    Node *node = args.node;

    if (!node)
        return true;

    uint32_t rot_args =
        arr_push(&state->args, &(LlrbArgs){.rotate = {.node = node->parent},
                                           .type = LLRB_ROTATE_ARGS});
    Operation rot_op = {
        .action = NULL, .args_index = rot_args, .timer = state->time + 1000};

    // TODO: Should these be their own ops?
    if (llrb_is_node_red(node->right) && !llrb_is_node_red(node->left)) {
        // node = llrb_rotate_left(node);
        rot_op.action = llrb_rotate_left;
        arr_push(&state->operations, &rot_op);
    } else if (llrb_is_node_red(node->left) &&
               llrb_is_node_red(node->left->left)) {
        // node = llrb_rotate_right(node);
        rot_op.action = llrb_rotate_right;
        arr_push(&state->operations, &rot_op);
    } else if (llrb_is_node_red(node->left) && llrb_is_node_red(node->right)) {
        // llrb_flip_colors(node);
        uint32_t flip_args =
            arr_push(&state->args, &(LlrbArgs){.flip_colors = {.node = node},
                                               .type = LLRB_FLIP_COLORS_ARGS});

        Operation flip_op = {.action = llrb_flip_colors,
                             .args_index = flip_args,
                             .timer = state->time + 1000};
        arr_push(&state->operations, &flip_op);
    }

    uint32_t up_args =
        arr_push(&state->args, &(LlrbArgs){.up = {.node = node->parent},
                                           .type = LLRB_UP_ARGS});

    Operation up_op = {
        .action = llrb_up, .args_index = up_args, .timer = state->time + 2000};
    arr_push(&state->operations, &up_op);

    return true;
}

bool llrb_new_node(TreeState *state, uint32_t args_index) {
    LlrbNewNodeArgs args =
        (&arr_get(&state->args, args_index, LlrbArgs))->new_node;

    Node *parent = args.parent;
    int32_t value = args.value;
    uint32_t side = args.side;

    Node *node = malloc(sizeof(Node));
    memset(node, 0, sizeof(Node));

    node->value = value;
    node->parent = parent;

    if (parent) {
        if (side == LLRB_LEFT)
            parent->left = node;
        else
            parent->right = node;
    } else {
        state->root = node;
    }

    // TODO: Make coordinates based
    Vec2 p = {0.0f, 0.2f};

    if (parent) {
        p = arr_get(&state->points, parent->point_index, Vec2);

        if (side == LLRB_LEFT) {
            p.x -= 0.1f;
        } else {
            p.x += 0.1f;
        }

        p.y += 0.2f;
    }

    node->point_index = arr_push(&state->points, &p);

    arr_push(&state->circles,
             &(Circle){.center = node->point_index,
                       .radius = 0.1f,
                       .color = (node->color == NODE_RED) ? COLOR_RED
                                                          : COLOR_BLACK});

    if (parent)
        arr_push(&state->lines, &(Line){.start = parent->point_index,
                                        .end = node->point_index,
                                        .color = COLOR_GREEN});

    return true;
}

bool llrb_insert(TreeState *state, uint32_t args_index) {
    LlrbInsertArgs args =
        (&arr_get(&state->args, args_index, LlrbArgs))->insert;
    Node *node = args.node;
    int32_t value = args.value;

    Operation op = {0};

    //
    //     if (value == node->value)
    //         goto up;

    if (value == node->value) {
        op.action = llrb_up;
        op.args_index = -1;
        op.timer = state->time + 1000;
    }

    //     else if (value < node->value)
    //         node->left = llrb_insert(node->left, value);
    else {
        LlrbInsertArgs insert_args = {.value = value};
        insert_args.node = (value < node->value) ? node->left : node->right;

        if (insert_args.node) {
            uint32_t arg =
                arr_push(&state->args, &(LlrbArgs){.insert = insert_args,
                                                   .type = LLRB_INSERT_ARGS});

            op.action = llrb_insert;
            op.args_index = arg;
            op.timer = state->time + 1000;
        } else {
            uint32_t new_args =
                arr_push(&state->args,
                         &(LlrbArgs){.new_node = {.parent = node,
                                                  .value = value,
                                                  .side = (value < node->value)
                                                              ? LLRB_LEFT
                                                              : LLRB_RIGHT},
                                     .type = LLRB_NEW_NODE_ARGS});
            op.action = llrb_new_node;
            op.args_index = new_args;
            op.timer = state->time + 1000;
        }
    }

    arr_push(&state->operations, &op);
    return true;
}

// Final step of the push operation
bool llrb_change_root_color(TreeState *state, uint32_t args_index) {
    if (state->operations.length != 1)
        return false;

    state->root->color = NODE_BLACK;
    return true;
}

bool llrb_push(TreeState *state, uint32_t args_index) {
    LlrbPushArgs args = (&arr_get(&state->args, args_index, LlrbArgs))->push;

    // Node *node = llrb_insert(root, value);
    if (state->root) {
        uint32_t arg = arr_push(
            &state->args,
            &(LlrbArgs){.insert = {.node = state->root, .value = args.value},
                        .type = LLRB_INSERT_ARGS});

        Operation insert = {
            .action = llrb_insert, .args_index = arg, .timer = 0};
        arr_push(&state->operations, &insert);
    } else {

        uint32_t new_args = arr_push(
            &state->args,
            &(LlrbArgs){.new_node = {.parent = NULL, .value = args.value},
                        .type = LLRB_NEW_NODE_ARGS});
        Operation create = {
            .action = llrb_new_node,
            .args_index = new_args,
            .timer = state->time + 1000,
        };
        arr_push(&state->operations, &create);
    }

    // node->color = NODE_BLACK;
    Operation flip_root = {
        .action = llrb_change_root_color, .args_index = -1, .timer = 0};
    arr_push(&state->operations, &flip_root);

    return true;
}
#endif
