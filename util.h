#ifndef CCOMP_UTIL_H
#define CCOMP_UTIL_H

#define ASSERT(cond) \
    if (!(cond)) { \
        fprintf(stderr, "Assertion failed at %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        exit(2); \
    } \

char* read_file(const char* path);
#endif //CCOMP_UTIL_H
