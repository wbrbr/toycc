#include <stdlib.h>
#include <stdint.h>

struct dynarray {
    void* data;
    size_t capacity; // in number of elements
    size_t length;

    size_t element_size;
};

void dynarray_init(struct dynarray* arr, size_t element_size);
void dynarray_init_with_capacity(struct dynarray* arr, size_t element_size, size_t capacity);
void dynarray_init_with_length(struct dynarray* arr, size_t element_size, size_t length, void* x);
void dynarray_destroy(struct dynarray* arr);
void dynarray_push(struct dynarray* arr, void* x);
void* dynarray_get(const struct dynarray* arr, size_t index);
void dynarray_set(struct dynarray* arr, size_t index, void* x);
size_t dynarray_length(const struct dynarray* arr);
void dynarray_pop(struct dynarray* arr);
void dynarray_map(struct dynarray* arr, void (*func)(void* ptr));
