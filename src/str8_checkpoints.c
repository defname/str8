#include "str8_checkpoints.h"
#include <stdlib.h>  // malloc, realloc
#include <stdint.h>
#include <string.h>  // memcpy
#include "str8_header.h"
#include "str8_simd.h"
#include "str8_debug.h"

/** @brief Return the offset from list start to idx in bytes. */
STATIC INLINE size_t checkpoints_entry_offset(size_t idx) {
    if (idx <= MAX_2BYTE_INDEX) {
        return (idx) * sizeof(uint16_t);
    }
    if (idx <= MAX_4BYTE_INDEX) {
        return MAX_2BYTE_INDEX * sizeof(uint16_t) +
               ((idx) - MAX_2BYTE_INDEX) * sizeof(uint32_t);
    }
    return MAX_2BYTE_INDEX * sizeof(uint16_t) +
           (MAX_4BYTE_INDEX - MAX_2BYTE_INDEX) * sizeof(uint32_t) +
           ((idx) - MAX_4BYTE_INDEX) * sizeof(uint64_t);
}

/**
 * @brief Return a pointer to the begin of the list.
 */
STATIC INLINE void *checkpoints_list(str8 str) {
    uint8_t type = STR8_TYPE(str);
    size_t table_count = str8cap(str)/CHECKPOINTS_GRANULARITY;
    // the list contains an entry for each TABLE_GRANULARITY bytes
    size_t table_bytesize = checkpoints_entry_offset(table_count);
    return ((char*)str) - (1 + 3 * STR8_FIELD_SIZE(type)) - table_bytesize;
}

/**
 * @brief Return a pointer to a list entry.
 * 
 * @param list A pointer to the end of the list (as returned by checkpoints_list())
 * @param idx Index of the list entry to return.
 * @returns A pointer to the list entry.
 */
STATIC INLINE void *checkpoints_entry(void *list, size_t idx) {
    return (char*)list + checkpoints_entry_offset(idx);
}

/** @brief Read the value of a table entry and return it. */
STATIC INLINE size_t read_entry(void *list, size_t idx) {
    void *entry = checkpoints_entry(list, idx);
    if (idx <= MAX_2BYTE_INDEX) {
        return *(uint16_t*)entry;
    }
    if (idx <= MAX_4BYTE_INDEX) {
        return *(uint32_t*)entry;
    }
    return *(uint64_t*)entry;
}

/** @brief Write the value to a table entry. */
STATIC INLINE void write_entry(void *list, size_t idx, size_t value) {
    void *entry = checkpoints_entry(list, idx);
    if (idx <= MAX_2BYTE_INDEX) {
        *(uint16_t*)entry = (uint16_t)value;
    }
    else if (idx <= MAX_4BYTE_INDEX) {
        *(uint32_t*)entry = (uint32_t)value;
    }
    else {
        *(uint64_t*)entry = (uint64_t)value;
    }
}

uint8_t str8_analyze(
    const char *str,
    size_t max_bytes,
    str8_analyze_config config, 
    str8_analyze_results *results)
{
    results->list = config.list;
    results->list_capacity = config.list_capacity;
    results->list_size = 0;
    results->size = 0;
    results->length = 0;
    results->list_created = false;

    void *list_pointer = results->list;

    for (;;) {
        size_t max_chunk_size = results->size + CHECKPOINTS_GRANULARITY < max_bytes
            ? CHECKPOINTS_GRANULARITY
            : max_bytes - results->size;
        
        if (max_chunk_size == 0) {
            break;
        }
        
        size_t chunk_size = str8_size_simd(str + results->size, max_chunk_size);
        size_t chunk_len = str8_count_chars_simd(str + results->size, chunk_size);
        results->size += chunk_size;
        results->length += chunk_len;

        size_t idx = results->list_size;

        // increase the table if necessary
        if (idx == results->list_capacity) {
            // grow fast to avoid allocations. this is just temporary anyways
            size_t new_capacity = results->list_capacity * 2;
            void *new_list = NULL;
            if (results->list_created) {
                new_list = realloc(results->list, new_capacity);
                if (!new_list) {
                    return 1;
                }
            }
            else {
                new_list = malloc(new_capacity);
                if (!new_list) {
                    return 1;
                }
                // the initial stack list should always be initiated with
                // MAX_2BYTE_INDEX uint16_t entries.
                memcpy(new_list, results->list, checkpoints_entry_offset(results->list_size));
                results->list_created = true;
            }
            results->list = new_list;
            results->list_capacity = new_capacity;
            list_pointer = results->list + checkpoints_entry_offset(results->list_size);
        }

        if (chunk_size < max_chunk_size) {  // NULL byte was found in chunk
            break;
        }

        if (idx <= MAX_2BYTE_INDEX) {
            *(uint16_t*)list_pointer = (uint16_t)results->length;
            list_pointer += sizeof(uint16_t);
        }
        else if (idx <= MAX_4BYTE_INDEX) {
            *(uint32_t*)list_pointer = (uint32_t)results->length;
            list_pointer += sizeof(uint32_t);
        }
        else {
            *(uint64_t*)list_pointer = (uint64_t)results->length;
            list_pointer += sizeof(uint64_t);
        }
        results->list_size++; 
    }

    return 0;
}