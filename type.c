#include "type.h"
#include "toycc.h"

struct Type Type_int()
{
    struct Type type;
    type.kind = TYPE_INT;
    type.size = 4;
    type.alignment = 4;

    return type;
}

struct Type Type_float()
{
    struct Type type;
    type.kind = TYPE_FLOAT;
    type.size = 4;
    type.alignment = 4;

    return type;
}

struct Type Type_void()
{
    struct Type type;
    type.kind = TYPE_VOID;
    type.size = 0;
    type.alignment = 0;

    return type;
}
