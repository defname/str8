#ifndef STR8_HEADER_H
#define STR8_HEADER_H

#include "str8.h"
#include <stddef.h>

#define STR8_TYPE0  0
#define STR8_TYPE1  1
#define STR8_TYPE2  2
#define STR8_TYPE4  3
#define STR8_TYPE8  4

#define STR8_TYPE(str) (((char*)(str))[-1] & 0x03)
#define STR8_IS_ASCII(str) !!(((char*)(str))[-1] & 0x80)
#define STR8_FIELD_SIZE(type) \
    ( \
        (type) == STR8_TYPE1 ? 1 : \
        (type) == STR8_TYPE2 ? 2 : \
        (type) == STR8_TYPE4 ? 4 : \
        (type) == STR8_TYPE8 ? 8 : \
        0 \
    )

#define STR8_TYPE0_SIZE(str) (size_t)(((char*)str)[-1] >> 3)

size_t str8len(str8 str);
size_t str8size(str8 str);
size_t str8cap(str8 str);

void str8setlen(str8 str, size_t length);
void str8setsize(str8 str, size_t size);
void str8setcap(str8 str, size_t capacity);

#endif
