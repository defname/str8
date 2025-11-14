#define TEST_INIT { srand(time(NULL)); }

#include "acutest.h"
#include "test_helper.h"
#include <stdlib.h>   // rand(), srand()
#include <stdbool.h>  // bool
#include <string.h>  // strnlen()
#include <time.h>  // time()

#include "src/str8_simd.h"

void verify_is_ascii(const char *s, bool expected, const char *descr) {
    TEST_CASE(descr);
    size_t size = strlen(s);
    bool result = is_ascii(s, size);
    TEST_CHECK_EQUAL(result, expected, "%d", "is_ascii");
}

void test_is_ascii(void) {
    verify_is_ascii("Hello World", true, "ASCII String");
    verify_is_ascii("Hello € World", false, "String with Euro sign");
    verify_is_ascii("äöü", false, "String with Umlauts");
    verify_is_ascii("", true, "Empty string");
}

void test_is_ascii_random(void) {
    for (int i=0; i<100; i++) {
        char *s = generate_random_string(ascii_charset, ascii_charset_size, rand() % 10000);
        verify_is_ascii(s, true, s);
        free(s);
    }
    for (int i=0; i<100; i++) {
        char *s = generate_random_string(utf8_charset, utf8_charset_size, rand() % 10000 + 1);
        verify_is_ascii(s, false, s);
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

void verify_count(const char *s, size_t size, const char *descr) {
    TEST_CASE(descr);
    size_t simd_result = count_chars(s, size);
    size_t scalar_result = count_chars_scalar(s, size);
    TEST_CHECK_EQUAL(simd_result, scalar_result, "%zu", "character count");
}

void test_count(void) {
    verify_count("", 0, "Empty String (0 bytes)");
    verify_count("TEST", 4, "\"TEST\" (4 bytes)");
    verify_count("TES€", 6, "\"TES€\" (6 bytes)");
}

void test_count_random(void) {
    size_t max_str_len = 100000;
    for (int i=0; i<100; i++) {
        char *s = generate_random_string(utf8_charset, utf8_charset_size, rand() % max_str_len);
        verify_count(s, strlen(s), s);
        free(s);
    }
}

const char *lookup_scalar(const char *str, size_t size, size_t idx) {
    size_t char_count = 0;
    const char *end = str + size;
    for (; str < end; str++) {
        if (((unsigned char)*str & 0xC0) != 0x80) {
            if (char_count == idx) {
                return str;
            }
            char_count++;
        }
    }
    return NULL;
}

void check_lookup(const char *s, size_t idx) {
    size_t size = strlen(s);
    const char *scalar_result = lookup_scalar(s, size, idx);
    const char *simd_result = lookup_idx(s, size, idx);
    TEST_CHECK_EQUAL(simd_result, scalar_result, "%p", "result");
}

void test_lookup(void) {
    TEST_CASE("ASCII Lookup");
    check_lookup("TEST 12345", 0);
    check_lookup("TEST 12345", 5);
    check_lookup("TEST ABCDEFGHIJKLMOPQRSTUVWXYZ 12345", 31);

    TEST_CASE("UTF8 Lookup");
    check_lookup("TEST ABCDEFGHIJKLMOPQRSTUVW€€€ 12345", 31);
    check_lookup("Fooo€bar", 4);
    check_lookup("Fooo€bar", 5);

    TEST_CASE("Out-of-range Lookup");
    check_lookup("TEST ABC", 100);
}

void test_lookup_random(void) {
    for (int i=0; i<100; i++) {
        size_t len = rand() % 100000;
        char *s = generate_random_string(utf8_charset, utf8_charset_size, len);
        size_t char_count = count_chars(s, strlen(s));

        for (int j=0; j<10; j++) { 
            if (char_count > 0) {
                size_t idx = rand() % char_count;
                check_lookup(s, idx);
            }
        }
        free(s);
    }
}

TEST_LIST = {
    { "SIMD: is_ascii", test_is_ascii },
    { "SIMD: is_ascii (Random)", test_is_ascii_random },
    { "SIMD: Character Count", test_count },
    { "SIMD: Character Count (Random)", test_count_random },
    { "SIMD: Lookup", test_lookup },
    { "SIMD: Lookup (Random)", test_lookup_random },
    { NULL, NULL }
};
