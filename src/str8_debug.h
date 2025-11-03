#ifndef STR8_DEBUG_H
#define STR8_DEBUG_H

#if defined(DEBUG)

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#define STATIC
#define INLINE

/* str8_header.h */
size_t get_field(void *field, uint8_t type);
bool value_in_range(uint8_t type, size_t value);
void set_field(void *field, uint8_t type, size_t value);
void *len_field(str8 str, uint8_t type);
void *size_field(str8 str, uint8_t type);
void *cap_field(str8 str, uint8_t type);

/* str8_checkpoints.h */
size_t checkpoints_entry_offset(size_t idx);
void *checkpoints_list(str8 str);
void *checkpoints_entry(void *list, size_t idx);
size_t read_entry(void *list, size_t idx);
void write_entry(void *list, size_t idx, size_t value);

/* str8_memory.h */
size_t calc_total_size(uint8_t type, bool ascii, size_t capacity);
uint8_t type_from_capacity(size_t cap);

#else
#define STATIC static
#define INLINE inline
#endif

#endif