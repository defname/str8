/**
 * @file str8_simd.h
 * @brief Public interface for SIMD-accelerated string analysis functions.
 */

#ifndef STR8_SIMD_H
#define STR8_SIMD_H

#include <stddef.h>

/**
 * @brief Calculates the length of a string in bytes, stopping at the first null
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
 * @brief Counts the number of UTF-8 characters in a buffer of a known size.
 *
 * This function assumes the input is valid UTF-8 and counts all bytes that are
 * not continuation bytes.
 *
 * @param str The string buffer to analyze.
 * @param size The exact size of the buffer in bytes.
 * @return The number of UTF-8 characters.
 */
size_t str8_count_chars_simd(const char *str, size_t size);

#endif // STR8_SIMD_H
