#include <stdio.h>
#include "../hashmap.h"
#include "../util.h"

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    struct hashmap map;
    hashmap_init(&map, sizeof(int));

    int v;
    ASSERT(!hashmap_get(&map, "a", &v));
    v = 5;
    hashmap_set(&map, "a", &v);
    int w = -1;
    ASSERT(hashmap_get(&map, "a", &w));
    ASSERT(w == 5);

    v = 12345;
    hashmap_set(&map, "b", &v);
    ASSERT(hashmap_get(&map, "b", &w));
    ASSERT(w == 12345)

    ASSERT(hashmap_get(&map, "a", &w));
    ASSERT(w == 5);

    v = -12;
    hashmap_set(&map, "a",&v);
    ASSERT(hashmap_get(&map, "a", &w));
    ASSERT(w == -12);

    hashmap_destroy(&map);

    puts("Passed.");
    return 0;
}