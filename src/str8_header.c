#include "str8_header.h"
#include <stdint.h>
#include <stdbool.h>
#include "str8_debug.h"
#include "str8_simd.h"

/**
 * @brief Get the content of a header field.
 * 
 * @param field Pointer to the header field.
 * @param type Type of the header.
 * @returns The value of the field is returned
 */
STATIC INLINE size_t get_field(void *field, uint8_t type) {
    switch (type) {
        case STR8_TYPE1:
            return *(uint8_t*)field;
        case STR8_TYPE2:
            return *(uint16_t*)field;
        case STR8_TYPE4:
            return *(uint32_t*)field;
        case STR8_TYPE8:
            return *(uint64_t*)field;
        default:
            return 0;
    }
}

/** @brief Check if the value is in range for type */
STATIC INLINE bool value_in_range(uint8_t type, size_t value) {
    switch (type) {
        case STR8_TYPE0:
            return value < (1 << 5);
        case STR8_TYPE1:
            return value <= UINT8_MAX;
        case STR8_TYPE2:
            return value <= UINT16_MAX;
        case STR8_TYPE4:
            return value <= UINT32_MAX;
        default:
            return true;
    }
}

/**
 * @brief Set the content of a header field.
 * 
 * @param field Pointer to the field to set.
 * @param type Type of the header.
 * @param value Value to write to the field.
 */
STATIC INLINE void set_field(void *field, uint8_t type, size_t value) {
#ifdef DEBUG
    assert(value_in_range(type, value));
#endif
    switch (type) {
        case STR8_TYPE1:
            *(uint8_t*)field = (uint8_t)value;
            break;
        case STR8_TYPE2:
            *(uint16_t*)field = (uint16_t)value;
            break;
        case STR8_TYPE4:
            *(uint32_t*)field = (uint32_t)value;
            break;
        case STR8_TYPE8:
            *(uint64_t*)field = (uint64_t)value;
            break;
        default:
            break;
    }
}

/** @brief Return a pointer to the length field in the header of str (not for type 0!!!). */
STATIC INLINE void *len_field(str8 str, uint8_t type) {
    return ((char*)str) - (1 + 3 * STR8_FIELD_SIZE(type));
}

/**
 * @brief Return a pointer to the size field in the header of str (not for type 0!!!). */
STATIC INLINE void *size_field(str8 str, uint8_t type) {
    return ((char*)str) - (1 + STR8_FIELD_SIZE(type));
}

/** @brief Return a pointer to the capacity field in the header of str (not for type 0!!!). */
STATIC INLINE void *cap_field(str8 str, uint8_t type) {
    return ((char*)str) - (1 + 2 * STR8_FIELD_SIZE(type));
}

size_t str8len(str8 str) {
    uint8_t type = STR8_TYPE(str);
    if (type == STR8_TYPE0) {
        return count_chars(str, STR8_TYPE0_SIZE(str));
    }
    if (STR8_IS_ASCII(str)) {
        return str8size(str);
    }
    void *field = len_field(str, type);
    return get_field(field, type);
}

size_t str8size(str8 str) {
    uint8_t type = STR8_TYPE(str);
    if (type == STR8_TYPE0) {
        return STR8_TYPE0_SIZE(str);
    }
    void *field = size_field(str, type);
    return get_field(field, type);
}

size_t str8cap(str8 str) {
    uint8_t type = STR8_TYPE(str);
    if (type == STR8_TYPE0) {
        return STR8_TYPE0_SIZE(str);
    }
    void *field = cap_field(str, type);
    return get_field(field, type);
}

void str8setlen(str8 str, size_t length) {
    uint8_t type = STR8_TYPE(str);
    if (type == STR8_TYPE0 || STR8_IS_ASCII(str)) {
        return;
    }
    void *field = len_field(str, type);
    set_field(field, type, length);
}

void str8setsize(str8 str, size_t size) {
    uint8_t type = STR8_TYPE(str);
    if (type == STR8_TYPE0) {
        ((uint8_t*)str)[-1] = (((uint8_t)size) << 3);  // type is 0
        return;
    }
    void *field = size_field(str, type);
    set_field(field, type, size);
}

void str8setcap(str8 str, size_t capacity) {
    uint8_t type = STR8_TYPE(str);
    if (type == STR8_TYPE0) {
        return;
    }
    void *field = cap_field(str, type);
    set_field(field, type, capacity);
}
