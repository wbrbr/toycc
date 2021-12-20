#include <stdio.h>
#include <string.h>
#include "hashmap.h"
#include "xxhash.h"

void hashmap_init(struct hashmap* map, size_t element_size)
{
    hashmap_init_ex(map, element_size, 0.75f, 256);
}

void hashmap_init_ex(struct hashmap* map, size_t element_size, float load_factor, size_t capacity)
{
    map->load_factor = load_factor;
    map->capacity = capacity;
    map->element_size = element_size;
    map->entries = calloc(capacity, sizeof(struct hashmap_entry));
}

void hashmap_set(struct hashmap* map, char* key, void* val)
{
    size_t len_key = strlen(key);
    uint64_t hash = XXH3_64bits(key, len_key);

    for (size_t i = 0; i < map->capacity; i++) {
        struct hashmap_entry* entry = &map->entries[(hash+i) % map->capacity];
        if (entry->key == NULL) {
            entry->key = calloc(1, len_key+1);
            memcpy(entry->key, key, len_key+1);
            entry->value = calloc(1, map->element_size);
            memcpy(entry->value, val, map->element_size);
            return;
        } else if (strcmp(entry->key, key) == 0) {
            memcpy(entry->value, val, map->element_size);
        }
    }

    fprintf(stderr, "Hashtable is full\n");
    exit(1);
}

bool hashmap_get(const struct hashmap* map, const char* key, void* val)
{
    size_t len_key = strlen(key);
    uint64_t hash = XXH3_64bits(key, len_key);

    for (size_t i = 0; i < map->capacity; i++) {
        struct hashmap_entry* entry = &map->entries[(hash+i) % map->capacity];
        if (entry->key == NULL) {
            break;
        }

        if (strcmp(key, entry->key) == 0) {
            memcpy(val, entry->value, map->element_size);
        }
    }

    return false;
}