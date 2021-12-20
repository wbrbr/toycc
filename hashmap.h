#ifndef CCOMP_HASHMAP_H
#define CCOMP_HASHMAP_H
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

struct hashmap_entry {
    char* key;
    void* value;
};

struct hashmap {
    struct hashmap_entry* entries;
    float load_factor;
    size_t capacity;
    size_t element_size;
};

void hashmap_init(struct hashmap* map, size_t element_size);
void hashmap_init_ex(struct hashmap* map, size_t element_size, float load_factor, size_t capacity);
bool hashmap_get(const struct hashmap* map, const char* key, void* val);

#endif //CCOMP_HASHMAP_H
