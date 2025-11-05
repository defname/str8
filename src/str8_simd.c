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
    #define SIMD_ACTIVE
    #define SIMD_TYPE           __m128i
    #define SIMD_SETZERO        _mm_setzero_si128()
    #define SIMD_SET(val)       _mm_set1_epi8(val)
    #define SIMD_LOAD_BYTES(p)  _mm_loadu_si128(p)
    #define SIMD_CMP_EQ(a, b)   _mm_cmpeq_epi8(a, b)
    #define SIMD_BIT_AND(a, b)  _mm_and_si128(a, b)
#elif defined(__aarch64__)
    #include <arm_neon.h> // 64-bit ARM NEON
    #define SIMD_ACTIVE
    #define SIMD_TYPE           uint8x16_t
    #define SIMD_SETZERO        vdupq_n_u8(0)
    #define SIMD_SET(val)       vdupq_n_u8(val)
    #define SIMD_LOAD_BYTES(p)  vld1q_u8(p)
    #define SIMD_CMP_EQ(a, b)   vceqq_u8(a, b)
    #define SIMD_BIT_AND(a, b)  vandq_u8(a, b)
#endif

// Define PAGE_SIZE for the page boundary check
#ifndef PAGE_SIZE
#define PAGE_SIZE 4096 // Common page size, can be queried via sysconf(_SC_PAGESIZE)
#endif

static inline bool is_safe_to_read_16_bytes(const char* p) {
    uintptr_t addr = (uintptr_t)p;
    return (addr & ~(PAGE_SIZE - 1)) == ((addr + 15) & ~(PAGE_SIZE - 1));
}

#ifdef SIMD_ACTIVE
__attribute__((__no_sanitize_address__))
static inline SIMD_TYPE load_bytes_insecure(const char *p) {
    return SIMD_LOAD_BYTES((const void*)p);
}
#endif

size_t str8_count_chars_simd(const char *str, size_t size) {
    size_t i = 0;
    size_t char_count = 0;
    const size_t step = 16;

#ifdef SIMD_ACTIVE
    const SIMD_TYPE mask_c0 = SIMD_SET(0xC0);
    const SIMD_TYPE mask_80 = SIMD_SET(0x80);

    while (i + step <= size) {
        SIMD_TYPE chunk = load_bytes_insecure(str + i);
        SIMD_TYPE top_bits = SIMD_BIT_AND(chunk, mask_c0);
        SIMD_TYPE cont_bytes = SIMD_CMP_EQ(top_bits, mask_80);
        
        int cont_count = 0;
    #if defined(__x86_64__) || defined(_M_X64)
        int mask = _mm_movemask_epi8(cont_bytes);
        #ifdef __POPCNT__
            cont_count = _mm_popcnt_u32(mask);
        #else
            for(int j=0; j<16; j++) if((mask>>j)&1) cont_count++;
        #endif
    #elif defined(__aarch64__)
        uint8x16_t ones_and_zeros = vshrq_n_u8(cont_bytes, 7);
        cont_count = vaddvq_u8(ones_and_zeros);
    #endif
        char_count += (step - cont_count);
        i += step;
    }
#endif

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

#ifdef SIMD_ACTIVE
    const SIMD_TYPE zero = SIMD_SETZERO;

    while (max_size == 0 || i + step <= max_size) {
        if (!is_safe_to_read_16_bytes(str + i)) {
            if ((unsigned char)str[i] == '\0') { return i; }
            i++;
            continue;
        }
        
        SIMD_TYPE chunk = load_bytes_insecure(str + i);
        SIMD_TYPE equal_zero = SIMD_CMP_EQ(chunk, zero);

    #if defined(__x86_64__) || defined(_M_X64)
        int mask = _mm_movemask_epi8(equal_zero);
        if (mask != 0) {
            #ifdef __GNUC__
                return i + __builtin_ctz(mask);
            #else
                unsigned long null_idx;
                _BitScanForward(&null_idx, mask);
                return i + null_idx;
            #endif
        }
    #elif defined(__aarch64__)
        if (vmaxvq_u8(equal_zero) != 0) {
            break;
        }
    #endif
        i += step;
    }
#endif

    while ((max_size == 0 || i < max_size) && str[i] != '\0') {
        i++;
    }
    return i;
}

size_t str8_scan_simd(const char *str, size_t max_size, size_t *first_non_ascii_pos) {
    size_t i = 0;
    const size_t step = 16;

#ifdef SIMD_ACTIVE
    const SIMD_TYPE zero = SIMD_SETZERO;

    while (max_size == 0 || i + step <= max_size) {
        if (!is_safe_to_read_16_bytes(str + i)) {
            if ((unsigned char)str[i] == '\0') { return i; }
            if (*first_non_ascii_pos == (size_t)-1 && (unsigned char)str[i] > 127) {
                *first_non_ascii_pos = i;
            }
            i++;
            continue;
        }
        SIMD_TYPE chunk = load_bytes_insecure(str + i);

    #if defined(__x86_64__) || defined(_M_X64)
        if (*first_non_ascii_pos == (size_t)-1) {
            int non_ascii_mask = _mm_movemask_epi8(chunk);
            if (non_ascii_mask != 0) {
                #ifdef __GNUC__
                    *first_non_ascii_pos = i + __builtin_ctz(non_ascii_mask);
                #else
                    unsigned long index;
                    _BitScanForward(&index, non_ascii_mask);
                    *first_non_ascii_pos = i + index;
                #endif
            }
        }
        
        SIMD_TYPE equal_zero = SIMD_CMP_EQ(chunk, zero);
        int zero_mask = _mm_movemask_epi8(equal_zero);
        if (zero_mask != 0) {
            #ifdef __GNUC__
                return i + __builtin_ctz(zero_mask);
            #else
                unsigned long index;
                _BitScanForward(&index, zero_mask);
                return i + index;
            #endif
        }
    #elif defined(__aarch64__)
        const SIMD_TYPE non_ascii_bit = SIMD_SET(0x80);
        SIMD_TYPE non_ascii_bytes = SIMD_BIT_AND(chunk, non_ascii_bit);
        SIMD_TYPE equal_zero_bytes = SIMD_CMP_EQ(chunk, zero);
        SIMD_TYPE combined = vorrq_u8(non_ascii_bytes, equal_zero_bytes);

        if (vmaxvq_u8(combined) != 0) {
            break;
        }
    #endif
        i += step;
    }
#endif

    while ((max_size == 0 || i < max_size) && str[i] != '\0') {
        if (*first_non_ascii_pos == (size_t)-1 && (unsigned char)str[i] > 127) {
            *first_non_ascii_pos = i;
        }
        i++;
    }
    return i;
}

const char *str8_lookup_idx_simd(const char *str, size_t idx, size_t max_bytes) {
    size_t i = 0;
    size_t char_count = 0;
    const size_t step = 16;

#ifdef SIMD_ACTIVE
    const SIMD_TYPE mask_c0 = SIMD_SET(0xC0);
    const SIMD_TYPE mask_80 = SIMD_SET(0x80);

    while (i + step <= max_bytes) {
        SIMD_TYPE chunk = load_bytes_insecure(str + i);
        SIMD_TYPE top_bits = SIMD_BIT_AND(chunk, mask_c0);
        SIMD_TYPE cont_bytes = SIMD_CMP_EQ(top_bits, mask_80);
        
        int cont_count = 0;
    #if defined(__x86_64__) || defined(_M_X64)
        int mask = _mm_movemask_epi8(cont_bytes);
        #ifdef __POPCNT__
            cont_count = _mm_popcnt_u32(mask);
        #else
            for(int j=0; j<16; j++) if((mask>>j)&1) cont_count++;
        #endif
    #elif defined(__aarch64__)
        uint8x16_t ones_and_zeros = vshrq_n_u8(cont_bytes, 7);
        cont_count = vaddvq_u8(ones_and_zeros);
    #endif

        if (char_count + step - cont_count > idx) {
            break;
        }
        char_count += (step - cont_count);
        i += step;
    }
#endif

    while (i < max_bytes) {
        if (((unsigned char)str[i] & 0xC0) != 0x80) {
            if (char_count == idx) {
                return str + i;
            }
            char_count++;
        }
        i++;
    }
    
    return NULL;
}