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
 * @brief Calculates the length of a string in bytes and simultaneously checks
 *        ob der String nur aus ASCII-Zeichen besteht.
 *
 * This function is a SIMD-accelerated combination of `strnlen` and an
 * ASCII validation. It stops at the first null terminator or after
 * `max_size` bytes have been examined.
 *
 * @param str The string to be analyzed.
 * @param max_size The maximum number of bytes to scan. If 0, scans until
 *                 the null terminator.
 * @param ascii A pointer to a boolean value, which is set to `false` if a
 *              non-ASCII character (>127) is found.
 * @return The number of bytes in the string (up to `max_size`).
 */
size_t str8_scan_simd(const char *str, size_t max_size, bool *ascii);


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
