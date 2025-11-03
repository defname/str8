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
#elif defined(__aarch64__)
    #include <arm_neon.h> // 64-bit ARM NEON
#endif

// Define PAGE_SIZE for the page boundary check
#ifndef PAGE_SIZE
#define PAGE_SIZE 4096 // Common page size, can be queried via sysconf(_SC_PAGESIZE)
#endif

/**
 * @brief Checks if a 16-byte read from address `p` will cross a page boundary.
 *        This is crucial to prevent segfaults when reading slightly past allocated memory.
 */
static inline bool is_safe_to_read_16_bytes(const char* p) {
    uintptr_t addr = (uintptr_t)p;
    // Check if the start address and the end address (addr + 15) are on the same page.
    return (addr & ~(PAGE_SIZE - 1)) == ((addr + 15) & ~(PAGE_SIZE - 1));
}


/**
 * @brief Load chunk into SIMD register.
 * 
 * This function is marked `no_sanitize_address` because it might read
 * slightly past the end of a string (but within the same memory page).
 */
__attribute__((__no_sanitize_address__))
// --- x86 SSE/AVX Implementation ---
#if defined(__x86_64__) || defined(_M_X64)
static inline __m128i load_bytes_insecure(const char *p) {
    return _mm_loadu_si128((const __m128i*)p);
}
// --- ARM NEON (AArch64) Implementation ---
#elif defined(__aarch64__)
static inline uint8x16_t load_bytes_insecure(const char *p) {
    return vld1q_u8((const uint8_t*)p);
}
#endif


size_t str8_count_chars_simd(const char *str, size_t size) {
    size_t i = 0;
    size_t char_count = 0;
    const size_t step = 16;

// --- x86 SSE/AVX Implementation ---
#if defined(__x86_64__) || defined(_M_X64)
    const __m128i mask_c0 = _mm_set1_epi8(0xC0);
    const __m128i mask_80 = _mm_set1_epi8(0x80);

    while (i + step <= size) {
        // Use a direct, safe load as we are guaranteed to be within `size`.
        __m128i chunk = _mm_loadu_si128((const __m128i*)(str + i));
        __m128i top_bits = _mm_and_si128(chunk, mask_c0);
        __m128i cont_bytes = _mm_cmpeq_epi8(top_bits, mask_80);
        int mask = _mm_movemask_epi8(cont_bytes);
        #ifdef __POPCNT__
            int cont_count = _mm_popcnt_u32(mask);
        #else
            int cont_count = 0;
            for(int j=0; j<16; j++) if((mask>>j)&1) cont_count++;
        #endif
        char_count += (step - cont_count);
        i += step;
    }

// --- ARM NEON (AArch64) Implementation ---
#elif defined(__aarch64__)
    const uint8x16_t mask_c0 = vdupq_n_u8(0xC0);
    const uint8x16_t mask_80 = vdupq_n_u8(0x80);

    while (i + step <= size) {
        // Use a direct, safe load as we are guaranteed to be within `size`.
        uint8x16_t chunk = vld1q_u8((const uint8_t*)(str + i));
        uint8x16_t top_bits = vandq_u8(chunk, mask_c0);
        uint8x16_t cont_bytes_mask = vceqq_u8(top_bits, mask_80);
        uint8x16_t ones_and_zeros = vshrq_n_u8(cont_bytes_mask, 7);
        int cont_count = vaddvq_u8(ones_and_zeros);
        char_count += (step - cont_count);
        i += step;
    }
#endif

    // --- Scalar Fallback Loop ---
    while (i < size) {
        if (((unsigned char)str[i] & 0xC0) != 0x80) {
            char_count++;
        }
        i++;
    }

    return char_count;
}


size_t str8_size_simd(const char *str, size_t max_size) {
    size_t i = 0;
    const size_t step = 16;

// --- x86 SSE/AVX Implementation ---
#if defined(__x86_64__) || defined(_M_X64)
    const __m128i zero = _mm_setzero_si128();

    while (max_size == 0 || i + step <= max_size) {
        // check if the boundary of a memory page is in reach
        if (!is_safe_to_read_16_bytes(str + i)) {
            if ((unsigned char)str[i] == '\0') {
                return i;
            }
            i++;
            continue;
        }
        __m128i chunk = load_bytes_insecure(str + i);
        __m128i equal_zero = _mm_cmpeq_epi8(chunk, zero);
        int mask = _mm_movemask_epi8(equal_zero);

        if (mask != 0) {
            // Found a null byte in this chunk. Find its exact index.
            #ifdef __GNUC__
                return i + __builtin_ctz(mask);
            #else
                unsigned long null_idx;
                _BitScanForward(&null_idx, mask);
                return i + null_idx;
            #endif
        }
        i += step;
    }

// --- ARM NEON (AArch64) Implementation ---
#elif defined(__aarch64__)
    const uint8x16_t zero = vdupq_n_u8(0);

    while (max_size == 0 || i + step <= max_size) {
        if (!is_safe_to_read_16_bytes(str + i)) {
            if ((unsigned char)str[i] == '\0') {
                return i;
            }
            i++;
            continue;
        }
        uint8x16_t chunk = load_bytes_insecure(str + i);
        uint8x16_t equal_zero_mask = vceqq_u8(chunk, zero);
        
        // Check if any byte in the mask is non-zero (i.e., 0xFF).
        // This indicates a null byte was found.
        if (vmaxvq_u8(equal_zero_mask) != 0) {
            // Found a null byte, but NEON has no easy `movemask`.
            // Fall back to a scalar check for this final chunk.
            break;
        }
        i += step;
    }
#endif

    // --- Scalar Fallback Loop ---
    // This part is reached if:
    // 1. The architecture is not x86 or AArch64.
    // 2. The SIMD loops finished without finding a null byte.
    // 3. The NEON loop found a chunk with a null byte and broke early.
    while (i < max_size && str[i] != '\0') {
        i++;
    }
    return i;
}