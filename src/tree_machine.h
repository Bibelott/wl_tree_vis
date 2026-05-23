#pragma once

#include "llrb.h"
#include "dyn_array.h"
#include "tree_vis_internal.h"
#include <stdint.h>

typedef struct {
    Array operations;
    Array args;
    Array points;
    Array circles;
    Array lines;
    Node *root;
    uint32_t time;
} TreeState;

typedef struct Op {
    bool (*action)(TreeState*, uint32_t);
    uint32_t args_index;
    // TODO: Support multiple types of trigger conditions
    uint32_t timer;
} Operation;

