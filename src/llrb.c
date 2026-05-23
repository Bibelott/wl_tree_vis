#include "llrb.h"
#include "dyn_array.h"
#include "tree_machine.h"
#include <stdint.h>
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

bool llrb_flip_colors(TreeState *state, void *data) {
    LlrbFlipColorsArgs *args = data;
    Node *node = args->node;

    node->color = !node->color;
    node->left->color = !node->left->color;
    node->right->color = !node->right->color;

    return true;
}

bool llrb_rotate_right(TreeState *state, void *data) {
    LlrbRotateArgs *args = data;
    Node *node = args->node;

    Node *x = node->left;
    node->left = x->right;
    x->right = node;
    x->color = node->color;
    node->color = NODE_RED;
    // return x;
    node = x;

    if (llrb_is_node_red(node->left) && llrb_is_node_red(node->right)) {
        // llrb_flip_colors(node);
        LlrbFlipColorsArgs flip_args = {.node = node};
        Operation flip_op = {.action = llrb_flip_colors,
                             .data = &flip_args,
                             .timer = state->time + 1000};
        arr_push(&state->operations, &flip_op);
    }

    return true;
}

bool llrb_rotate_left(TreeState *state, void *data) {
    LlrbRotateArgs *args = data;
    Node *node = args->node;

    Node *x = node->right;
    node->right = x->left;
    x->left = node;
    x->color = node->color;
    node->color = NODE_RED;
    // return x;
    node = x;

    if (llrb_is_node_red(node->left) && llrb_is_node_red(node->left->left)) {
        // node = llrb_rotate_right(node);
        LlrbRotateArgs rot_args = {.node = node};
        Operation rot_op = {.action = llrb_rotate_right,
                            .data = &rot_args,
                            .timer = state->time + 1000};
        arr_push(&state->operations, &rot_op);
    }
    if (llrb_is_node_red(node->left) && llrb_is_node_red(node->right)) {
        // llrb_flip_colors(node);
        LlrbFlipColorsArgs flip_args = {.node = node};
        Operation flip_op = {.action = llrb_flip_colors,
                             .data = &flip_args,
                             .timer = state->time + 1000};
        arr_push(&state->operations, &flip_op);
    }

    return true;
}

bool llrb_up(TreeState *state, void *data) {
    LlrbUpArgs *args = data;
    Node *node = args->node;

    if (!node)
        return true;

    LlrbRotateArgs rot_args = {.node = node->parent};
    Operation rot_op = {
        .action = NULL, .data = &rot_args, .timer = state->time + 1000};

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
        LlrbFlipColorsArgs flip_args = {.node = node};
        Operation flip_op = {.action = llrb_flip_colors,
                             .data = &flip_args,
                             .timer = state->time + 1000};
        arr_push(&state->operations, &flip_op);
    }

    LlrbUpArgs up_args = {.node = node->parent};
    Operation up_op = {
        .action = llrb_up, .data = &up_args, .timer = state->time + 2000};
    arr_push(&state->operations, &up_op);

    return true;
}

bool llrb_new_node(TreeState *state, void *data) {
    LlrbNewNodeArgs *args = data;
    Node *parent = args->parent;
    int32_t value = args->value;
    uint32_t side = args->side;

    Node *node = malloc(sizeof(Node));
    memset(node, 0, sizeof(Node));

    node->value = value;
    node->point_index = -1;
    node->parent = parent;

    if (parent) {
        if (side == LLRB_LEFT)
            parent->left = node;
        else
            parent->right = node;
    } else {
        state->root = node;
    }

    return true;
}

bool llrb_insert(TreeState *state, void *data) {
    LlrbInsertArgs *args = data;
    Node *node = args->node;
    int32_t value = args->value;

    Operation op = {0};

    //
    //     if (value == node->value)
    //         goto up;

    if (value == node->value) {
        op.action = llrb_up;
        op.data = NULL;
        op.timer = state->time + 1000;
    }

    //     else if (value < node->value)
    //         node->left = llrb_insert(node->left, value);
    else {
        LlrbInsertArgs insert_args = {.value = value};
        insert_args.node = (value < node->value) ? node->left : node->right;

        op.action = llrb_insert;
        op.data = &insert_args;
        op.timer = state->time + 1000;

        if (!insert_args.node) {
            LlrbNewNodeArgs new_args = {.parent = node, .value = value};
            op.action = llrb_new_node;
            op.data = &new_args;
            op.timer = state->time + 1000;
        }
    }

    arr_push(&state->operations, &op);
    return true;
}

// Final step of the push operation
bool llrb_change_root_color(TreeState *state, void *data) {
    if (state->operations.length != 1)
        return false;

    state->root->color = NODE_BLACK;
    return true;
}

bool llrb_push(TreeState *state, void *data) {
    LlrbPushArgs *args = data;

    // Node *node = llrb_insert(root, value);
    if (state->root) {
        LlrbInsertArgs insert_args = {.node = state->root,
                                      .value = args->value};
        Operation insert = {
            .action = llrb_insert, .data = &insert_args, .timer = 0};
        arr_push(&state->operations, &insert);
    } else {

        LlrbNewNodeArgs new_args = {.parent = NULL, .value = args->value};
        Operation create = {
            .action = llrb_new_node,
            .data = &new_args,
            .timer = state->time + 1000,
        };
        arr_push(&state->operations, &create);
    }

    // node->color = NODE_BLACK;
    Operation flip_root = {
        .action = llrb_change_root_color, .data = NULL, .timer = 0};
    arr_push(&state->operations, &flip_root);

    return true;
}
#endif
