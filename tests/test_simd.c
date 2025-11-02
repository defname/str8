#define TEST_INIT { srand(time(NULL)); }

#include "acutest.h"
#include "test_helper.h"
#include <string.h>  // strnlen()
#include <stdlib.h>   // rand(), srand()
#include <time.h>  // time()

#include "src/str8_simd.h"

bool simd_active() {
#if defined(__x86_64__) || defined(_M_X64)
    return true;
#elif defined(__aarch64__)
    return true;
#else
    return false;
#endif
}

void verify_size(const char *s, size_t max_size, const char *descr) {
    TEST_CASE(descr);
    size_t simd_result = str8_size_simd(s, max_size);
    size_t scalar_result = strnlen(s, max_size);
    
    TEST_CHECK_EQUAL(scalar_result, simd_result, "%zu", "size");
}

void test_size(void) {
    TEST_ASSERT(simd_active());
    verify_size("TEST", 10, "\"TEST\" (max_size: 10)");
    verify_size("TEST", 4, "\"TEST\" (max_size: 4)");
    verify_size("TEST", 2, "\"TEST\" (max_size: 2)");
    verify_size("TEST", 0, "\"TEST\" (max_size: 0)");
    verify_size("", 10, "Empty String (max_size: 10)");
}

void test_size_random(void) {
    size_t max_str_len = 1000000;
    TEST_ASSERT(simd_active());
    for (int i=0; i<100; i++) {
        char *s = generate_random_string(utf8_charset, utf8_charset_size, rand() % max_str_len);
        verify_size(s, max_str_len + 10, s);
        free(s);
    }
}

size_t count_chars_scalar(const char *str, size_t size) {
    size_t char_count = 0;
    for (size_t i=0; i<size; i++) {
        if (((unsigned char)str[i] & 0xC0) != 0x80) {
            char_count++;
        }
    }
    return char_count;
}

void verify_count(const char *s, size_t size, size_t expected, const char *descr) {
    TEST_CASE(descr);
    size_t simd_result = str8_count_chars_simd(s, size);
    TEST_CHECK_EQUAL(simd_result, expected, "%zu", "character count");
}

void test_count(void) {
    TEST_ASSERT(simd_active());
    verify_count("", 0, 0, "Empty String (0 bytes)");
    verify_count("TEST", 0, 0, "\"TEST\" (0 bytes)");
    verify_count("TEST", 4, 4, "\"TEST\" (4 bytes)");
    verify_count("TEST", 3, 3, "\"TEST\" (3 bytes)");
    verify_count("TES€", 6, 4, "\"TES€\" (6 bytes)");
    verify_count("TES€", 5, 4, "\"TES€\" (4 bytes)");
}

void test_count_random(void) {
    size_t max_str_len = 1000000;
    TEST_ASSERT(simd_active());
    for (int i=0; i<100; i++) {
        char *s = generate_random_string(utf8_charset, utf8_charset_size, rand() % max_str_len);
        size_t size = str8_size_simd(s, max_str_len + 10);
        verify_count(s, size, count_chars_scalar(s, size), s);
        free(s);
    }
}


TEST_LIST = {
    { "SIMD: Size", test_size },
    { "SIMD: Size (Random)", test_size_random },
    { "SIMD: Character Count", test_count },
    { "SIMD: Character Count (Random)", test_count_random },
    { NULL, NULL }
};
