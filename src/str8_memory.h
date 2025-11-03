#ifndef STR8_MEMORY
#define STR8_MEMORY

#include "str8.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "str8_debug.h"

typedef void*(*str8_allocator)(size_t);
typedef void*(*str8_reallocator)(void *, size_t);
typedef void(*str8_deallocator)(void *);


str8 str8_allocate(uint8_t type, bool ascii, size_t capacity, str8_allocator alloc);
str8 str8new(const char *str);
str8 str8newsize(const char *str, size_t max_size);
void str8free(str8 str);
str8 str8grow(str8 str, size_t new_capacity, bool utf8);

#endif