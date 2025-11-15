/**
 * @file str8_simd.h
 * @brief Public interface for SIMD-accelerated string analysis functions.
 */

#ifndef STR8_SIMD_H
#define STR8_SIMD_H

#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Check if there is a non-ASCII character in str.
 *
 * @param str Pointer to the string to check.
 * @param size Size in bytes of str (strlen(str)).
 *
 * @returns true if there is no non-ASCII character found, false otherwise
 */
bool is_ascii(const char *str, size_t size);

/**
 * @brief Count the number of characters in str.
 *
 * @param str pointer to the str.
 * @param size Length of str in bytes.
 * 
 * @returns Number of characters in str.
 */
size_t count_chars(const char *str, size_t size);

/**
 * @brief Return a pointer to the idx' character.
 * 
 * The function assumes that str is valid UTF-8 and idx is in bound.
 * All non continuation bytes are counted until idx is reached.
 * 
 * @param str The string buffer to count.
 * @param size The size of the string buffer in bytes.
 * @param target_idx The character index to look for.
 * @return A pointer to the position in str, or NULL if not found.
 */
const char *lookup_idx(const char *str, size_t size, size_t target_idx);

#endif // STR8_SIMD_H
