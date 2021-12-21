#include "dynarray.h"
#include <string.h>
#include <stdio.h>
#include "util.h"

void dynarray_init(struct dynarray* arr, size_t element_size) {
    arr->element_size = element_size;
    arr->capacity = 0;
    arr->length = 0;
    arr->data = NULL;
}

void dynarray_init_with_capacity(struct dynarray* arr, size_t element_size, size_t capacity)
{
    arr->element_size = element_size;
    arr->capacity = capacity;
    arr->length = 0;
    arr->data = calloc(capacity, element_size);
}

void dynarray_init_with_length(struct dynarray* arr, size_t element_size, size_t length, void* x)
{
    dynarray_init_with_capacity(arr, element_size, length);
    arr->length = length;
    for (size_t i = 0; i < length; i++) {
        memcpy(arr->data+i*element_size, x, element_size);    
    }
}

void dynarray_destroy(struct dynarray* arr)
{
    free(arr->data);
}

void dynarray_push(struct dynarray* arr, void* x)
{
    if (arr->length == arr->capacity) {
        arr->capacity = (arr->capacity == 0) ? 2 : 2*arr->capacity;
        arr->data = realloc(arr->data, arr->capacity*arr->element_size);
        
        if (arr->data == NULL) {
            fprintf(stderr, "Failed to grow dynamic array (realloc)\n");
            exit(1);
        }
    }
    
    memcpy(arr->data+arr->length*arr->element_size, x, arr->element_size);
    arr->length++;
}

void* dynarray_get(const struct dynarray* arr, size_t index)
{
    ASSERT(index < arr->length)
    return arr->data+index*arr->element_size;
}

void dynarray_set(struct dynarray* arr, size_t index, void* x)
{
    ASSERT(index < arr->length)
    memcpy(arr->data+index*arr->element_size, x, arr->element_size);
}

size_t dynarray_length(const struct dynarray* arr)
{
    return arr->length;
}

void dynarray_pop(struct dynarray* arr)
{
    ASSERT(arr->length > 0)
    arr->length--;
}

void dynarray_map(struct dynarray* arr, void (*func)(void* ptr))
{
    for (size_t i = 0; i < arr->length; i++) {
        func(arr->data+i*arr->element_size);
    }
}
