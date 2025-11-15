#include "str8_memory.h"
#include <stdlib.h>
#include <string.h>
#include "str8_header.h"
#include "str8_checkpoints.h"
#include "str8_simd.h"

/**
 * @brief Calculate the total number of bytes needed for the header.
 */
STATIC size_t calc_header_size(uint8_t type, bool ascii, size_t capacity) {
    if (type == STR8_TYPE0) {
        return 1;
    }
    uint8_t field_size = STR8_FIELD_SIZE(type);
    // type field + size field + capacity field
    size_t size = 1 + 2 * field_size;
    if (ascii) {
        return size;
    }
    // + length field
    size += field_size;
    if (type == STR8_TYPE1) {
        // type 1 does not have a checkpoints list
        return size;
    }
    // size of checkpoints list
    size += checkpoints_list_total_size(capacity);
    return size;
}

STATIC INLINE void str8init(str8 str, uint8_t type, bool ascii, size_t capacity) {
    str[0] = '\0';
    str[-1] = type;
    if (type != STR8_TYPE0 && !ascii) {
        str[-1] |= 0x80;
    }
    str8setsize(str, 0);
    str8setlen(str, 0);
    str8setcap(str, capacity);
}

/** @brief Allocate memory return an initialized str8. */
str8 str8_allocate(uint8_t type, bool ascii, size_t capacity, str8_allocator alloc) {
    size_t header_size = calc_header_size(type, ascii, capacity);
    void *mem = alloc(header_size + capacity + 1);  // + '\0'
    if (!mem) {
        return NULL;
    }
    str8 str = (char*)mem + header_size;
    str8init(str, type, ascii, capacity);
    return str;
}

STATIC INLINE str8 str8new_type0_(const char *str, size_t size, str8_allocator alloc) {
    str8 new = str8_allocate(STR8_TYPE0, false, size, alloc);
    if (!new) {
        return NULL;
    }
    memcpy(new, str, size);
    new[size] = '\0';
    str8setsize(new, size);
    return new;
}

STATIC INLINE uint8_t type_from_capacity(size_t cap) {
    if (cap <= 31) {
        return STR8_TYPE0;
    }
    if (cap <= UINT8_MAX) {
        return STR8_TYPE1;
    }
    if (cap <= UINT16_MAX) {
        return STR8_TYPE2;
    }
    if (cap <= UINT32_MAX) {
        return STR8_TYPE4;
    }
    return STR8_TYPE8;
}

STATIC INLINE str8 str8newsize_(const char *str, size_t max_size, str8_allocator alloc) {
    size_t size = strnlen(str, max_size && max_size < 32 ? max_size : 32);
    if (size < 32) {
        return str8new_type0_(str, size, alloc);
    }

    uint16_t list[MAX_2BYTE_INDEX + 1];
    str8_analyze_config config = {
        .list = list,
        .list_capacity = MAX_2BYTE_INDEX + 1
    };
    str8_analyze_results results;

    int error = str8_analyze(str, max_size, config, &results);
    if (error != 0) {
        if (results.list_created) {
            free(results.list);
        }
        return NULL;
    }
    uint8_t type = type_from_capacity(results.size);
    bool ascii = (results.length == results.size);

    str8 new = str8_allocate(type, ascii, results.size, alloc);
    if (!new) {
        if (results.list_created) {
            free(results.list);
        }
    }
    memcpy(new, str, results.size);
    new[results.size] = '\0';
    str8setsize(new, results.size);

    if (!ascii) {
        str8setlen(new, results.length);
        void *checkpoints_list = checkpoints_list_ptr(new);
        if (checkpoints_list) {
            size_t table_size = results.list_created ? checkpoints_list_total_size(results.size) : results.list_size * 2;
            memcpy(checkpoints_list, results.list, table_size);
        }
    }
    if (results.list_created) {
        free(results.list);
    }
    return new;
}

str8 str8new(const char *str) {
    return str8newsize_(str, 0, malloc);
}

str8 str8newsize(const char *str, size_t max_size) {
    return str8newsize_(str, max_size, malloc);
}

STATIC INLINE void *get_memory_block_start(str8 str) {
    size_t header_size = calc_header_size(STR8_TYPE(str), STR8_IS_ASCII(str), str8cap(str));
    return str - header_size;
}

STATIC INLINE void str8free_(str8 str, str8_deallocator dealloc) {
    uint8_t type = STR8_TYPE(str);
    if (type == STR8_TYPE0) {
        dealloc(&str[-1]);
        return;
    }
    void *mem = get_memory_block_start(str);
    dealloc(mem);
}

void str8free(str8 str) {
    str8free_(str, free);
}

STATIC INLINE str8 str8grow_(str8 str, size_t new_capacity, bool utf8, str8_reallocator realloc) {
    uint8_t type = STR8_TYPE(str);
    size_t capacity = str8cap(str);
    
    if (new_capacity <= capacity) {
        return str;
    }

    uint8_t new_type = type_from_capacity(new_capacity);
    if (new_type == STR8_TYPE0) {
        // if a string capacity is increased it's likely that it
        // happens again, type 0 is insufficient for that
        new_type = STR8_TYPE1;
    }
    bool ascii = STR8_IS_ASCII(str);
    size_t size = str8size(str);
    size_t length = size;
    
    if (!ascii) {
        length = str8len(str);
    }

    size_t header_size = calc_header_size(type, ascii, capacity);
    size_t new_header_size = calc_header_size(new_type, ascii && !utf8, new_capacity);

    void *mem = get_memory_block_start(str);
    void *new_mem = realloc(mem, new_header_size + new_capacity + 1);
    if (!new_mem) {
        return NULL;
    }

    // str might be dangling after realloc
    str = (char*)new_mem + header_size;

    // amount mem needs to be moved to the right, to align correctly
    // with the new header size
    size_t memory_diff = new_header_size - calc_header_size(type, ascii, capacity);

    // 1.  The header size did not change, so everything is still in place
    
    if (memory_diff == 0) {
        str8setcap(str, new_capacity);
        return str;
    }

    // 2.  The header size changed, so move the string to the correct position
    
    memmove(str + memory_diff, str, size + 1);
    // fix str
    str += memory_diff;
    // update fields
    str[-1] = new_type;
    if (!ascii || utf8) {
        str[-1] |= 0x80;
    }
    str8setsize(str, size);
    str8setlen(str, length);
    str8setcap(str, new_capacity);

    return str;
}

str8 str8grow(str8 str, size_t new_capacity, bool utf8) {
    return str8grow_(str, new_capacity, utf8, realloc);
}

STATIC INLINE size_t calc_cap_with_prealloc(size_t new_size) {
    size_t realloc = new_size / 2;
    realloc = realloc > STR8_MAX_PREALLOC ? STR8_MAX_PREALLOC : realloc;
    return new_size + realloc;
}

str8 str8append_(str8 str, const char *other, size_t max_size, str8_reallocator realloc) {
    if (other == NULL || *other == '\0') {
        return str;
    }
    size_t size = str8size(str);
    size_t capacity = str8cap(str);
    bool ascii = STR8_IS_ASCII(str);
    size_t length = str8len(str);

    size_t other_size = max_size ? strnlen(other, max_size) : strlen(other);

    size_t new_size = size + other_size;
    bool new_ascii = is_ascii(other, other_size);

    size_t new_capacity = capacity;
    if (new_size > new_capacity) {
        new_capacity = calc_cap_with_prealloc(new_size);
    }

    uint8_t new_type = type_from_capacity(new_capacity);
    if (new_type == STR8_TYPE0) {
        new_type = STR8_TYPE1;
    }

    str8 new = str8grow_(str, new_capacity, !new_ascii, realloc);
    if (!new) {
        return NULL;
    }

    memcpy(new + size, other, other_size);
    new[new_size] = '\0';
    str8setsize(new, new_size);

    if (STR8_TYPE(new) == STR8_TYPE1) {
        if (!new_ascii) {
            str8setlen(new, count_chars(new, new_size));
        }
        return new;
    }

    if (ascii && !new_ascii) {
        // build table for original str
        void *list_entry = checkpoints_list_ptr(new);
        for (size_t idx=0; idx<size/CHECKPOINTS_GRANULARITY; idx++) {
            size_t val = (idx+1) * CHECKPOINTS_GRANULARITY;
            if (idx <= MAX_2BYTE_INDEX) {
                *(uint16_t*)list_entry = (uint16_t)val;
                list_entry += sizeof(uint16_t);
            }
            else if (idx <= MAX_4BYTE_INDEX) {
                *(uint32_t*)list_entry = (uint32_t)val;
                list_entry += sizeof(uint32_t);
            }
            else {
                *(uint64_t*)list_entry = (uint64_t)val;
                list_entry += sizeof(uint64_t);
            }
        }
    }

    if (!new_ascii) {
        void *list = checkpoints_list_ptr(new);
        size_t new_length;
            
        size_t list_capacity = new_capacity / CHECKPOINTS_GRANULARITY;
        str8_analyze_config config = {
            .list = list,
            .list_capacity = list_capacity,
            .byte_offset = size,
            .list_start_idx = size / CHECKPOINTS_GRANULARITY,
            .char_idx_offset = length
        };
        str8_analyze_results results;
        int error = str8_analyze(other, max_size, config, &results);
        if (error != 0) {
            str8free(new);
            return NULL;
        }
        new_length = length + results.length;
        
        str8setlen(new, new_length);
    }

    return new;
}

str8 str8append(str8 str, const char *other) {
    return str8append_(str, other, 0, realloc);
}
