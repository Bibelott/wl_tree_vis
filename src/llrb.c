#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define NODE_RED 0
#define NODE_BLACK 1

typedef struct Node {
    int32_t value;
    uint32_t color;
    struct Node *left;
    struct Node *right;

    // TODO: Figure this shit out
    uint32_t point_index;
} Node;

Node *llrb_new_node(int32_t value) {
    Node *node = malloc(sizeof(Node));
    memset(node, 0, sizeof(Node));

    node->value = value;
    node->point_index = -1;
    // node->color = NODE_RED; // implicit since NODE_RED == 0

    return node;
}

int32_t llrb_is_node_red(Node *node) {
    if (node && node->color == NODE_RED)
        return 1;
    else
        return 0;
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
