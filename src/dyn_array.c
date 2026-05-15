#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ARR_INIT_SIZE 8
#define arr_get(arr, i, type) ((type *)((arr)->ay))[i]
#define arr_index(arr, i) ((arr)->ay)[i * (arr)->element_size]

typedef struct {
    uint32_t length;
    uint32_t capacity;
    uint32_t element_size;
    void *ay;
} Array;

Array arr_create(uint32_t elem_size) {
    assert(elem_size > 0);

    void *buffer = calloc(ARR_INIT_SIZE, elem_size);
    Array arr = {.length = 0, .capacity = ARR_INIT_SIZE, .element_size = elem_size, .ay = buffer};
    return arr;
}

// Increases array capacity to fit at least n additional elements
void arr_grow_by(Array *arr, uint32_t n) {
    assert(arr->capacity > 0);
    assert(arr->element_size > 0);

    if (arr->length + n <= arr->capacity) {
        return;
    }

    do {
        arr->capacity *= 2;
    } while (arr->capacity < arr->length + n);

    void *newArray = calloc(arr->element_size, arr->capacity);

    memcpy(newArray, arr->ay, arr->length * arr->element_size);

    free(arr->ay);

    arr->ay = newArray;
}

uint32_t arr_push(Array *arr, void *elem) {
    assert(arr->element_size > 0);

    arr_grow_by(arr, 1);

    memcpy(&arr_index(arr, arr->length), elem, arr->element_size);

    return arr->length++;
}

uint32_t arr_find(Array *arr, void *elem) {
    assert(arr->element_size > 0);

    for (uint32_t index = 0; index < arr->length; index++) {
        if (memcmp(&arr_index(arr, index), elem, arr->element_size))
            return index;
    }

    return -1;
}

uint32_t arr_push_unique(Array *arr, void *elem) {
    uint32_t found = arr_find(arr, elem);
    if (found == -1)
        return arr_push(arr, elem);

    return found;
}

uint32_t arr_pop(Array *arr) {
    if (arr->length == 0)
        return 0;

    return arr->length--;
}

void arr_free(Array *arr) {
    free(arr->ay);
}
