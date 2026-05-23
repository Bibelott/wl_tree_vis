#pragma once

#include "llrb.h"
#include "dyn_array.h"
#include "tree_vis_internal.h"
#include <stdint.h>

typedef struct {
    Array operations;
    Node *root;
    uint32_t time;
} TreeState;

typedef struct Op {
    bool (*action)(TreeState*, void*);
    // TODO: Will these be getting dropped?
    void *data;
    // TODO: Support multiple types of trigger conditions
    uint32_t timer;
} Operation;

