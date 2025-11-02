/**
 * @file str8_checkpoints.h
 * @brief Manages the variable-size checkpoints list.
 *
 * The checkpoints list is a packed data structure where each entry's size
 * (uint16_t, uint32_t, or uint64_t) depends on its index.
 */
#ifndef STR8_CHECKPOINTS_H
#define STR8_CHECKPOINTS_H

#include "str8.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef CHECKPOINTS_GRANULARITY
#define CHECKPOINTS_GRANULARITY 512
#endif

#define MAX_2BYTE_INDEX ((UINT16_MAX / CHECKPOINTS_GRANULARITY) - 1)
#define MAX_4BYTE_INDEX ((UINT32_MAX / CHECKPOINTS_GRANULARITY) - 1)
#define MAX_8BYTE_INDEX ((UINT64_MAX / CHECKPOINTS_GRANULARITY) - 1)

typedef struct {
    void *list;             //< Pointer to existing list of uint16_t entries
    size_t list_capacity;   //< Capacity of the list (should be MAX_2BYTE_INDEX if it's a temporary list on the stack)
    size_t byte_offset;     //< Offset in bytes where the anaylsis should assume to start
} str8_analyze_config;

typedef struct {
    void *list;
    size_t list_size;
    size_t list_capacity;
    size_t size;
    size_t length;
    bool list_created;
} str8_analyze_results;

void deinit_results(str8_analyze_results *results);

/**
 * @brief Analyze str and write the results to results.
 * 
 * Calculate the size in bytes and the number of characters in str.
 * Create a list checkpoints list. Use config.list to write the results to,
 * if capacity is reached allocate a longer list on the heap.
 * If a new list is created list_created will be true, false otherwise.
 * 
 * If the max_bytes choppes a multi-byte character it will not be in the list.
 */
uint8_t str8_analyze(
    const char *str,
    size_t max_bytes,
    str8_analyze_config config, 
    str8_analyze_results *results);

#endif