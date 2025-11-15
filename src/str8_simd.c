/**
 * @file str8_simd.c
 * @brief SIMD-accelerated implementations for string analysis.
 */

#include "str8_simd.h"
#include <string.h> // For strnlen fallback
#include <stdint.h>
#include <stdbool.h>

// Include SIMD intrinsics based on architecture
#if defined(__x86_64__) || defined(_M_X64)
    #include <immintrin.h> // x86 SSE/AVX
#endif

// Define PAGE_SIZE for the page boundary check
#ifndef PAGE_SIZE
#define PAGE_SIZE 4096 // Common page size, can be queried via sysconf(_SC_PAGESIZE)
#endif


static inline __attribute__((always_inline))
bool is_ascii_scalar(const char *str, size_t size) {
    const char *end = str + size;
    for (; str < end; str++) {
        if (__builtin_expect(*str & 0x80, 0)) {
            return false;
        }
    }
    return true;
}


static inline __attribute__((always_inline))
size_t count_chars_scalar(const char *str, size_t size) {
    const char *end = str + size;
    size_t count = 0;
    for (; str < end; str++) {
        count += (*str & 0xC0) != 0x80;
    }
    return count;
}

static inline __attribute__((always_inline))
const char *lookup_idx_scalar(const char *str, size_t size, size_t *char_count, size_t target_idx) {
    const char *end = str + size;
    for (; str < end; str++) {
        if ((*str & 0xC0) != 0x80) {
            if (*char_count == target_idx) {
                return str;
            }
            (*char_count)++;
        }
    }
    return NULL;
}

#if defined(__x86_64__) || defined(_M_X64)

/**
 * @brief Align p to n
 */
static inline __attribute__((always_inline))
const char *align_to(const char *p, size_t n) {
    return (const char*)(((uintptr_t)p + n - 1) & ~(n - 1));
}


/**
 * @brief Read 32 bytes from p. Address sanitizer deactivated for this function!
 *
 * p NEED to be aligned correctly to 32 bytes.
 */
__attribute__((__no_sanitize_address__))
static inline __attribute__((always_inline))
__m256i load_bytes_insecure(const char *p) {
    return _mm256_load_si256((const __m256i *)p);
}


/**
 * @brief Check if there is a non-ASCII character in str.
 *
 * @param str Pointer to the string to check. Needs to be aligned correctly!
 * @param size Size in bytes of str (strlen(str)).
 *
 * @returns true if there is no non-ASCII character found, false otherwise
 */
static inline __attribute__((always_inline))
bool is_ascii_avx2(const char *str, size_t size) {
    const char *end = str + size;
    for (; str < end; str += sizeof(__m256i)) {
        if (__builtin_expect(_mm256_movemask_epi8(load_bytes_insecure(str)), 0)) {
            return false;
        }
    }
    return true;
}


bool is_ascii(const char *str, size_t size) {
    const char *p = str;
    const char * const end = str + size;
    const size_t V = 32;

    // --- Scalar prefix to align p to a 32-byte boundary ---
    const char *aligned_p = align_to(p, V);
    if (aligned_p > end) {
        return is_ascii_scalar(p, end - p);
    }

    size_t prologue_len = aligned_p - p;
    if (!is_ascii_scalar(p, prologue_len)) {
        return false;
    }
    p += prologue_len;

    // --- SIMD main loop ---
    size_t remaining_size = end - p;
    size_t simd_len = remaining_size & ~(V - 1);
    if (!is_ascii_avx2(p, simd_len)) return false;
    p += simd_len;

    // --- Scalar tail ---
    return is_ascii_scalar(p, end - p);
}


/**
 * @brief Count the number of characters in str.
 *
 * @param str (Aligned!) pointer to the str.
 * @param size Length of str in bytes.
 * 
 * @returns Number of characters in str.
 */
static inline __attribute__((always_inline))
size_t count_chars_avx2(const char *str, size_t size) {
    const char *end = str + size;
    size_t continuous_count = 0;

    const __m256i mask_80 = _mm256_set1_epi8((char)0x80); // Bit 7
    const __m256i mask_C0 = _mm256_set1_epi8((char)0xC0); // Bits 7+6

    for (; str < end; str += sizeof(__m256i)) {
        __m256i chunk = load_bytes_insecure(str);
        __m256i top_bits = _mm256_and_si256(chunk, mask_C0);
        __m256i cont_bytes = _mm256_cmpeq_epi8(top_bits, mask_80);
        int cont_mask = _mm256_movemask_epi8(cont_bytes);
        continuous_count += __builtin_popcount(cont_mask);
    }
    return size - continuous_count;
}


size_t count_chars(const char *str, size_t size) {
    const char *p = str;
    const char * const end = str + size;
    size_t count = 0;
    const size_t V = 32;

    // --- Scalar prefix to align p to a 32-byte boundary ---
    const char *aligned_p = align_to(p, V);
    if (aligned_p > end) {
        return count_chars_scalar(str, size);
    }

    size_t prologue_len = aligned_p - p;
    count += count_chars_scalar(p, prologue_len);
    p += prologue_len;

    // --- SIMD main loop ---
    size_t remaining_size = end - p;
    size_t simd_len = remaining_size & ~(V - 1);
    count += count_chars_avx2(p, simd_len);
    p += simd_len;

    // --- Scalar tail ---
    count += count_chars_scalar(p, end - p);
    return count;
}


static inline __attribute__((always_inline))
const char *lookup_idx_avx2(const char *str, size_t size, size_t *char_count, size_t target_idx) {
    const char *end = str + size;
    const size_t step = sizeof(__m256i);

    const __m256i mask_c0 = _mm256_set1_epi8(0xC0);
    const __m256i mask_80 = _mm256_set1_epi8(0x80);

    for (; str < end; str += step) {
        __m256i chunk = load_bytes_insecure(str);
        __m256i top_bits = _mm256_and_si256(chunk, mask_c0);
        __m256i cont_bytes = _mm256_cmpeq_epi8(top_bits, mask_80);
        
        int mask = _mm256_movemask_epi8(cont_bytes);
        int chars_in_chunk = step - __builtin_popcount(mask);

        if (*char_count + chars_in_chunk > target_idx) {
            return str; // Return the beginning of the chunk where the char is.
        }
        *char_count += chars_in_chunk;
    }
    return NULL; // Not found in the SIMD part
}


const char *lookup_idx(const char *str, size_t size, size_t target_idx) {
    const char *p = str;
    const char * const end = str + size;
    size_t char_count = 0;
    const size_t V = sizeof(__m256i);

    // --- Scalar prefix to align p ---
    const char *aligned_p = align_to(p, V);
    if (aligned_p > end) {
        return lookup_idx_scalar(p, end - p, &char_count, target_idx);
    }

    size_t prologue_len = aligned_p - p;
    const char *result = lookup_idx_scalar(p, prologue_len, &char_count, target_idx);
    if (result) {
        return result;
    }
    p += prologue_len;

    // --- SIMD main loop ---
    size_t remaining_size = end - p;
    size_t simd_len = remaining_size & ~(V - 1);
    const char *chunk_start = lookup_idx_avx2(p, simd_len, &char_count, target_idx);
    p += simd_len;

    // --- Scalar tail ---
    if (chunk_start) {
        // The character is in the chunk found by AVX2. Find exact position.
        return lookup_idx_scalar(chunk_start, V, &char_count, target_idx);
    } else {
        // The character is in the remaining tail.
        return lookup_idx_scalar(p, end - p, &char_count, target_idx);
    }
}

#else

bool is_ascii(const char *str, size_t size) {
    return is_ascii_scalar(str, size);
}

size_t count_chars(const char *str, size_t size) {
    return count_chars_scalar(str, size);
}

const char *lookup_idx(const char *str, size_t size, size_t target_idx) {
    size_t char_count = 0;
    return lookup_idx_scalar(str, size, &char_count, target_idx);
}

#endif