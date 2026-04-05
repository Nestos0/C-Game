#include "utils.h"

String *string_new(int capacity)
{
    String *s = malloc(sizeof(String));
    s->data = malloc(capacity);
    s->len = 0;
    s->capacity = capacity;
    s->data[0] = '\0';
    return s;
}

// 释放 String
void string_free(String *s)
{
    if (s) {
        free(s->data);
        free(s);
    }
}
