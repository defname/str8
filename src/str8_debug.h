#ifndef STR8_DEBUG_H
#define STR8_DEBUG_H

#if defined(DEBUG)
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#define STATIC
#define INLINE

/* str8_header.h */
STATIC INLINE size_t get_field(void *field, uint8_t type);
STATIC INLINE bool value_in_range(uint8_t type, size_t value);
STATIC INLINE void set_field(void *field, uint8_t type, size_t value);
STATIC INLINE void *len_field(str8 str, uint8_t type);
STATIC INLINE void *size_field(str8 str, uint8_t type);
STATIC INLINE void *cap_field(str8 str, uint8_t type);

/* str8_checkpoints.h */
STATIC INLINE size_t checkpoints_entry_offset(size_t idx);
STATIC INLINE void *checkpoints_list(str8 str);
STATIC INLINE void *checkpoints_entry(void *list, size_t idx);
STATIC INLINE size_t read_entry(void *list, size_t idx);
STATIC INLINE void write_entry(void *list, size_t idx, size_t value);


#else
#define STATIC static
#define INLINE inline
#endif

#endif