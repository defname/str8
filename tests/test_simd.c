#define TEST_INIT { srand(time(NULL)); }

#include "acutest.h"
#include "test_helper.h"
#include <stdlib.h>   // rand(), srand()
#include <stdbool.h>  // bool
#include <string.h>  // strnlen()
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
    TEST_CASE("Any length");
    size_t simd_result = str8_size_simd("fooooo bar blub", 0);
    size_t scalar_result = strlen("fooooo bar blub");
    TEST_CHECK_EQUAL(scalar_result, simd_result, "%zu", "size");
    
    verify_size("TEST", 10, "\"TEST\" (max_size: 10)");
    verify_size("TEST", 4, "\"TEST\" (max_size: 4)");
    verify_size("TEST", 2, "\"TEST\" (max_size: 2)");
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


void verify_scan(const char *s, size_t max_size, const char *descr) {
    TEST_CASE(descr);
    size_t first_non_ascii_pos = (size_t)-1;
    size_t simd_result = str8_scan_simd(s, max_size, &first_non_ascii_pos);
    size_t scalar_result = strnlen(s, max_size);
    size_t scalar_first_non_ascii_pos = 0;
    for (;scalar_first_non_ascii_pos<scalar_result && s[scalar_first_non_ascii_pos] && (unsigned char)(s[scalar_first_non_ascii_pos]) < 128; scalar_first_non_ascii_pos++) {}
    if (scalar_first_non_ascii_pos == scalar_result) {
        scalar_first_non_ascii_pos = (size_t)-1;
    }
    TEST_CHECK_EQUAL(simd_result, scalar_result, "%zu", "size");
    TEST_CHECK_EQUAL(first_non_ascii_pos, scalar_first_non_ascii_pos, "%zu", "first non-ascii pos");
}

void test_scan(void) {
    TEST_ASSERT(simd_active());
    TEST_CASE("Any length");
    {
        size_t first_non_ascii_pos = (size_t)-1;
        size_t simd_result = str8_scan_simd("fooooo bar blub", 0, &first_non_ascii_pos);
        size_t scalar_result = strlen("fooooo bar blub");
        TEST_CHECK_EQUAL(scalar_result, simd_result, "%zu", "size");
        TEST_CHECK_EQUAL(first_non_ascii_pos, (size_t)-1, "%zu", "first non-ascii pos");
    }
    TEST_CASE("Any length with non-ASCII characters");
    {
        size_t first_non_ascii_pos = (size_t)-1;
        size_t simd_result = str8_scan_simd("fooooo€ bar blub", 0, &first_non_ascii_pos);
        size_t scalar_result = strlen("fooooo€ bar blub");
        TEST_CHECK_EQUAL(scalar_result, simd_result, "%zu", "size");
        TEST_CHECK_EQUAL(first_non_ascii_pos, (size_t)6, "%zu", "first non-ascii pos");
    }
    TEST_CASE("Any length with non-ASCII characters");
    {
        size_t first_non_ascii_pos = (size_t)-1;
        size_t simd_result = str8_scan_simd("€fooooo€ bar blub", 0, &first_non_ascii_pos);
        size_t scalar_result = strlen("€fooooo€ bar blub");
        TEST_CHECK_EQUAL(scalar_result, simd_result, "%zu", "size");
        TEST_CHECK_EQUAL(first_non_ascii_pos, (size_t)0, "%zu", "first non-ascii pos");
    }
    verify_scan("TEST", 10, "\"TEST\" (max_size: 10)");
    verify_scan("TEST", 4, "\"TEST\" (max_size: 4)");
    verify_scan("TEST", 2, "\"TEST\" (max_size: 2)");
    verify_scan("", 10, "Empty String (max_size: 10)");
}

void test_scan_random(void) {
    size_t max_str_len = 1000000;
    TEST_ASSERT(simd_active());
    for (int i=0; i<100; i++) {
        char *s = generate_random_string(utf8_charset, utf8_charset_size, rand() % max_str_len);
        verify_scan(s, max_str_len + 10, s);
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

void test_lookup(void) {
    TEST_ASSERT(simd_active());
    TEST_CASE("ASCII Lookup 1");
    {
        const char *s = "TEST 12345";
        size_t size = str8_size_simd(s, 0);

        const char *result = str8_lookup_idx_simd(s, 0, size);
        TEST_CHECK_EQUAL(result, s, "%p", "result");
    }
    TEST_CASE("ASCII Lookup 2");
    {
        const char *s = "TEST 12345";
        size_t size = str8_size_simd(s, 0);

        const char *result = str8_lookup_idx_simd(s, 5, size);
        TEST_CHECK_EQUAL(result, s+5, "%p", "result");
    }
    TEST_CASE("ASCII Lookup 3");
    {
        const char *s = "TEST ABCDEFGHIJKLMOPQRSTUVWXYZ 12345";
        size_t size = str8_size_simd(s, 0);

        const char *result = str8_lookup_idx_simd(s, 31, size);
        TEST_CHECK_EQUAL(result, s+31, "%p", "result");
    }

    TEST_CASE("UTF8 Lookup 1");
    {
        const char *s = "TEST ABCDEFGHIJKLMOPQRSTUVW€€€ 12345";
        size_t size = str8_size_simd(s, 0);

        const char *result = str8_lookup_idx_simd(s, 31, size);
        TEST_CHECK_EQUAL(result, s+37, "%p", "result");
    }
    TEST_CASE("UTF8 Lookup 2");
    {
        const char *s = "Fooo€bar";
        size_t size = str8_size_simd(s, 0);

        const char *result = str8_lookup_idx_simd(s, 5, size);
        TEST_CHECK_EQUAL(result, s+7, "%p", "result");
    }

    TEST_CASE("out-of-range Lookup 3");
    {
        const char *s = "TEST ABCDEFGHIJKLMOPQRSTUVW€€€ 12345";
        size_t size = str8_size_simd(s, 0);

        const char *result = str8_lookup_idx_simd(s, 100, size);
        TEST_CHECK_EQUAL(result, NULL, "%p", "result");
    }
}

const char *lookup_scalar(const char *str, size_t idx) {
    while (*str) {
        if ((((unsigned char)*str) & 0xC0) != 0x80) {
            if (idx == 0) {
                return str;
            }
            idx--;
        }
        str++;
    }
    return NULL;
}

void check_lookup(const char *s, size_t idx) {
    size_t size = str8_size_simd(s, 0);
    const char *scalar_result = lookup_scalar(s, idx);
    const char *simd_result = str8_lookup_idx_simd(s, idx, size);
    TEST_CHECK_EQUAL(simd_result, scalar_result, "%p", "result");
}

void test_lookup_random(void) {
    TEST_ASSERT(simd_active());
    for (int i=0; i<100; i++) {
        size_t len = rand() % 1000000;
        char *s = generate_random_string(utf8_charset, utf8_charset_size, len);
        TEST_CASE(s);
        size_t char_count = str8_count_chars_simd(s, len);

        for (int j=0; j<50; j++) { 
            size_t idx = rand() % char_count;
            check_lookup(s, idx);
        }
        free(s);
    }
}

TEST_LIST = {
    { "SIMD: Size", test_size },
    { "SIMD: Size (Random)", test_size_random },
    { "SIMD: Scan", test_scan },
    { "SIMD: Scan (Random)", test_scan_random },
    { "SIMD: Character Count", test_count },
    { "SIMD: Character Count (Random)", test_count_random },
    { "SIMD: Lookup", test_lookup },
    { "SIMD: Lookup (Random)", test_lookup_random },
    { NULL, NULL }
};
