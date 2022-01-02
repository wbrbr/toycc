#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

char* read_file(const char* path)
{
    FILE* fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "Failed to open %s: %s\n", path, strerror(errno));
        exit(1);
    }
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    rewind(fp);

    char* buf = malloc(size+1);

    if (fread(buf, 1, size, fp) != size) {
        fprintf(stderr, "Failed to read %s: %s\n", path, strerror(errno));
        exit(1);
    }

    buf[size] = 0;

    return buf;
}