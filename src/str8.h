#ifndef STR8_H
#define STR8_H

#include <stddef.h>

typedef char* str8;

str8 str8new(const char *str);
void str8free(str8 str);

const char *str8getchar(str8 str, size_t idx);

#endif
