#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define ARR_INIT_SIZE 8
#define arr_get(arr, i, type) ((type *)((arr)->ay))[i]
#define arr_len(arr, type) (((arr)->length) / sizeof(type))
#define arr_push(arr, elem) arr_push_impl((arr), &(elem), sizeof(elem))

// NOTE: length and capacity are in bytes
typedef struct {
    uint32_t length;
    uint32_t capacity;
    void *ay;
} Array;

void arr_grow_by(Array *arr, uint32_t elem_size) {
    if (arr->length + elem_size <= arr->capacity) {
        return;
    }

    if (arr->capacity == 0) {
        arr->capacity = ARR_INIT_SIZE * elem_size;
        arr->ay = calloc(ARR_INIT_SIZE, elem_size);

        return;
    }

    do {
        arr->capacity *= 2;
    } while (arr->capacity < arr->length + elem_size);

    void *newArray = calloc(1, arr->capacity);

    memcpy(newArray, arr->ay, arr->length);

    free(arr->ay);

    arr->ay = newArray;
}

uint32_t arr_push_impl(Array *arr, void *elem, uint32_t size) {
    uint32_t index = arr->length / size;

    arr_grow_by(arr, size);

    memcpy(&arr->ay[arr->length], elem, size);

    arr->length += size;

    return index;
}

void *arr_pop(Array *arr, uint32_t size) {
    arr->length -= size;
    return &arr->ay[arr->length];
}

void arr_free(Array *arr) {
    free(arr->ay);
}
