#include <stdlib.h>
#include <stdint.h>

struct dynarray {
    void* data;
    size_t capacity; // in number of elements
    size_t length;

    size_t element_size;
};

void dynarray_init(struct dynarray* arr, size_t element_size);
void dynarray_destroy(struct dynarray* arr);
void dynarray_push(struct dynarray* arr, void* x);
void* dynarray_get(const struct dynarray* arr, size_t index);
size_t dynarray_length(const struct dynarray* arr);
void dynarray_pop(struct dynarray* arr);
void dynarray_map(struct dynarray* arr, void (*func)(void* ptr));
