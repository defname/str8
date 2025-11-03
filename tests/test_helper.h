#ifndef TEST_HELPER_H
#define TEST_HELPER_H

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define TEST_CHECK_EQUAL(val, exp, format, name) \
    do { \
        TEST_CHECK((val) == (exp)); \
        TEST_MSG("Expected %s to be "format", but got "format, (name), (exp), (val)); \
        fflush(stderr); \
    } while (0);

#define TEST_CHECK_STR(val, exp) \
    do { \
        TEST_CHECK(strcmp((val), (exp)) == 0); \
        TEST_MSG("Expected to be \"%s\", but got \"%s\"", (exp), (val)); \
        fflush(stderr); \
    } while (0);


static const char *ascii_charset[] = { "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" } ;
size_t ascii_charset_size = sizeof(ascii_charset) / sizeof(ascii_charset[0]);

static const char *utf8_charset[] = { "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "€", "ä", "ö", "ü", "ß", "Ä", "Ö", "Ü" } ;
size_t utf8_charset_size = sizeof(utf8_charset) / sizeof(utf8_charset[0]);


__attribute__((unused))
static char *generate_random_string(const char *charset[], size_t charset_size, size_t length) {
    // Allocate a bit more space to be safe with multi-byte chars
    char *s = (char*)malloc(length + 8);
    if (!s) return NULL;
    
    size_t size = 0;

    // Enforce last character to make sure there is at least one utf8 character
    if (charset_size > 0 && length > 0) {
        size_t l = strlen(charset[charset_size-1]);
        if (length >= l) {
            memcpy(s, charset[charset_size-1], l);
            size += l;
        }
    }

    while (1) {
        const char *entry = charset[rand() % charset_size];
        int entry_len = strlen(entry);
        if (size + entry_len > length) {
            break;
        }
        memcpy(s + size, entry, entry_len);
        size += entry_len;
    }
    s[size] = '\0';
    return s;
}

#endif