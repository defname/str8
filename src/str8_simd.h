/**
 * @file str8_simd.h
 * @brief Public interface for SIMD-accelerated string analysis functions.
 */

#ifndef STR8_SIMD_H
#define STR8_SIMD_H

#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Calculate the length of a string in bytes, stop at the first null
 *        terminator or after `max_size` bytes.
 *
 * This is a SIMD-accelerated equivalent of `strnlen`.
 *
 * @param str The string to measure.
 * @param max_size The maximum number of bytes to scan.
 * @return The number of bytes in the string, up to `max_size`.
 */
size_t str8_size_simd(const char *str, size_t max_size);

/**
 * @brief Scans a string to find its length in bytes and the position of the
 *        first non-ASCII character.
 *
 * This is a SIMD-accelerated function that performs two tasks in a single pass:
 * 1. It calculates the string length in bytes (like `strnlen`).
 * 2. It finds the byte offset of the first character with a value > 127.
 *
 * @param str The string to be analyzed.
 * @param max_size The maximum number of bytes to scan. If 0, scans until the
 *                 null terminator.
 * @param first_non_ascii_pos A pointer to a size_t. The caller should initialize
 *                            this to a sentinel value (e.g., (size_t)-1). If a
 *                            non-ASCII character is found, this will be updated
 *                            with its byte position.
 * @return The number of bytes in the string (its size).
 */
size_t str8_scan_simd(const char *str, size_t max_size, size_t *first_non_ascii_pos);


/**
 * @brief Count the number of UTF-8 characters in a buffer of a known size.
 *
 * This function assumes the input is valid UTF-8 and counts all bytes that are
 * not continuation bytes.
 *
 * @param str The string buffer to analyze.
 * @param size The exact size of the buffer in bytes.
 * @return The number of UTF-8 characters.
 */
size_t str8_count_chars_simd(const char *str, size_t size);

/**
 * @brief Return a pointer to the idx' character.
 * 
 * The function assumes that str is valid UTF-8 and idx is in bound.
 * All non continuation bytes are counted until idx is reached.
 * 
 * @param str The string buffer to count.
 * @param idx The idx to count up to.
 * @param max_bytes Maxium number of bytes to iterate through.
 * @return A pointer to the position in str.
 */
const char *str8_lookup_idx_simd(const char *str, size_t idx, size_t max_bytes);

#endif // STR8_SIMD_H
