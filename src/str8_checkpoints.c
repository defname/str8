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
 * @brief Return a pointer to the begin of the list or NULL if str has no list.
 */
STATIC INLINE void *checkpoints_list(str8 str) {
    uint8_t type = STR8_TYPE(str);
    if (type <= STR8_TYPE1 || STR8_IS_ASCII(str)) {
        return NULL;
    }
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

    // When appending a string, the new checkpoint list must align with the
    // existing one. Checkpoints are created at intervals of CHECKPOINTS_GRANULARITY.
    // `byte_offset` is the size of the original string. We calculate how many
    // bytes are needed in the new string to reach the next checkpoint boundary.
    // This ensures the new checkpoints maintain the same global grid.
    // `first_rount_offset` is the remainder, determining the size of the first, partial chunk.
    size_t first_rount_offset = config.byte_offset % CHECKPOINTS_GRANULARITY;

    void *list_pointer = results->list + checkpoints_entry_offset(config.list_start_idx);

    for (;;) {
        size_t max_chunk_size = CHECKPOINTS_GRANULARITY - first_rount_offset;
        if (max_bytes != 0) {
            size_t remaining = results->size >= max_bytes ? 0 : max_bytes - results->size;
            if (remaining < max_chunk_size) {
                max_chunk_size = remaining;
            }
        }
        
        // this is needed only for the first iteration
        first_rount_offset = 0;

        // max_bytes was reached
        if (max_chunk_size == 0) {
            break;
        }
        
        // count size and length of the chunk
        size_t chunk_size = str8_size_simd(str + results->size, max_chunk_size);
        size_t chunk_len = str8_count_chars_simd(str + results->size, chunk_size);

        // update the results
        results->size += chunk_size;
        results->length += chunk_len;

        // quit if the end was reached (no other list entry necessary)
        if (chunk_size < max_chunk_size) {  // NULL byte was found in chunk
            break;
        }

        size_t idx = results->list_size;

        // increase the table if necessary
        if (idx == results->list_capacity) {
            if (config.list_start_idx != 0) {
                // If an existing table is extended, it cannot be reallocated!
                return 1;
            }
            // grow fast to avoid allocations. this is just temporary anyways
            size_t new_capacity = results->list_capacity * 2;
            void *new_list = NULL;
            if (results->list_created) {
                new_list = realloc(results->list, checkpoints_entry_offset(new_capacity));
                if (!new_list) {
                    return 1;
                }
            }
            else {
                new_list = malloc(checkpoints_entry_offset(new_capacity));
                if (!new_list) {
                    return 1;
                }
                // the initial stack list should always be initiated with
                // MAX_2BYTE_INDEX uint16_t entries.
                // When copying from the initial stack list, we must assume it was
                // created with fixed-size entries.
                memcpy(new_list, results->list, results->list_size * sizeof(uint16_t));
                results->list_created = true;
            }
            results->list = new_list;
            results->list_capacity = new_capacity;
            list_pointer = results->list + checkpoints_entry_offset(results->list_size + config.list_start_idx);
        }

        // append the list
        if (idx <= MAX_2BYTE_INDEX) {
            *(uint16_t*)list_pointer = (uint16_t)results->length + config.char_idx_offset;
            list_pointer += sizeof(uint16_t);
        }
        else if (idx <= MAX_4BYTE_INDEX) {
            *(uint32_t*)list_pointer = (uint32_t)results->length + config.char_idx_offset;
            list_pointer += sizeof(uint32_t);
        }
        else {
            *(uint64_t*)list_pointer = (uint64_t)results->length + config.char_idx_offset;
            list_pointer += sizeof(uint64_t);
        }
        results->list_size++; 
    }

    return 0;
}

size_t checkpoints_list_total_size(size_t capacity) {
    return checkpoints_entry_offset(capacity/CHECKPOINTS_GRANULARITY);
}

void *checkpoints_list_ptr(str8 str) {
    return checkpoints_list(str);
}

/**
 * @brief Return the list index of the entry with the highest value less upper_bound. 
 * 
 * Perform a upper-bound binary search on list.
 * 
 * @param list Pointer to the beginn of the list.
 * @param table_count Number of elements in list.
 * @param upper_bound The upper bound we are looking for.
 * @return The index of the ub list entry or table_count if all entries are larger.
 */
STATIC size_t find_entry_ub(void *list, size_t list_count, size_t upper_bound) {
    if (list_count == 0) {
        return 0;
    }
    size_t l = 0;
    size_t r = list_count;
    size_t result_idx = list_count;
    while (l < r) {
        size_t mid = l + (r - l) / 2;
        size_t val = read_entry(list, mid);
        if (val <= upper_bound) {
            result_idx = mid;
            l = mid + 1;
        }
        else {
            r = mid;
        }
    }
    return result_idx;
}

const char *str8getchar(str8 str, size_t idx) {
    if (idx == 0) {
        return str;
    }
    uint8_t type = STR8_TYPE(str);
    size_t size = str8size(str);
    if (idx >= size) {  // since character size >= 1
        return NULL;
    }
    if (type == STR8_TYPE0) {
        return str8_lookup_idx_simd(str, idx, size);
    }
    bool ascii = STR8_IS_ASCII(str);
    if (ascii) {
        return str + idx;
    }
    if (type == STR8_TYPE1) {  // type 1 does not have a list
        return str8_lookup_idx_simd(str, idx, size);
    }
    void *checkpoints_list = checkpoints_list_ptr(str);
    size_t list_count = size/CHECKPOINTS_GRANULARITY;

    size_t list_idx = find_entry_ub(checkpoints_list, list_count, idx);
    size_t byte_pos = 0;
    size_t idx_offset = 0;
    if (list_idx < list_count) {
        byte_pos = (list_idx + 1) * CHECKPOINTS_GRANULARITY;
        idx_offset = read_entry(checkpoints_list, list_idx);
    }
    return str8_lookup_idx_simd(str + byte_pos, idx - idx_offset, size - byte_pos);
}
