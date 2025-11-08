#include "acutest.h"
#include "test_helper.h"
#include "src/str8_header.h"
#include "src/str8_checkpoints.h"
#include "src/str8_memory.h"
#include "src/str8_simd.h"


void check_simple(const char *s) {
    TEST_CASE(s);


    str8 str = str8new(s);
    size_t size = str8_size_simd(s, 0);
    size_t length = str8_count_chars_simd(s, size);
    uint8_t type = type_from_capacity(size);

    TEST_CHECK(str);
    TEST_CHECK_EQUAL(STR8_TYPE(str), type, "%d", "type");
    TEST_CHECK_EQUAL(str8size(str), size, "%zu", "size");
    TEST_CHECK_EQUAL(str8len(str), length, "%zu", "length");
    TEST_CHECK_EQUAL(str8cap(str), size, "%zu", "capacity");

    if (type > STR8_TYPE1 && str[-1] & 0x80) {
        void *list = checkpoints_list_ptr(str);
        TEST_CHECK(list);
        TEST_MSG("Expected list not to be NULL.");
        size_t value = read_entry(list, 0);
        TEST_CHECK(value <= CHECKPOINTS_GRANULARITY);
        TEST_MSG("Expected the first entry to be <= %d but got %zu", CHECKPOINTS_GRANULARITY, value);
    }
    str8free(str);
}

void test_new_simple(void) {

    check_simple("TEST");
    check_simple("FOOOOOOOo asdajsd adjknasdlknasdlknasdlnaslalasndlanslasnd asda");
    check_simple("€");
    check_simple("");

    char s[80000];
    memset(s, 'A', 80000);
    s[79999] = '\0';
    check_simple(s);

    memset(s, 'A', 80000);
    memcpy(s + 312, "€€", 6);
    s[79999] = '\0';
    check_simple(s);


    memset(s, 'A', 80000);
    memcpy(s + 312, "€€", 6);
    s[400] = '\0';
    check_simple(s);
}

void test_failed_ranom_tests(void) {
    const char *s1 =
    "Ü2vy€üqßvwCc4zUVUQufWTsKXcduwRLßN6uGßWOrakFÜVh6x9iC0ü7s59wtzh4wV42v0"
    "HdaTnR48ZTA2üßKd33c6tvoMtWpfGW6hZaügRZjKMJvUFÜ9D2ärpAR2TvlYßcYd1€Ö8W"
    "€ÜoQUVEtVvWRAib0ZÄBpiulkmogfzßüDhjb1ÜGc6üäFwnSeöPSj€öGogVuxoCsRJOSEN"
    "sTNoYmWä50ÜCAvGG1VNQj€4MxDpäq6SUTnczzYEmHP1beBIaepQAtPöQmvJOvvrßIFH8"
    "mäo5VjaZLIZ19yjlüp1tXF8a0hÄDMEF€KT9nÄ€gUA6PEyäPFiKYÜjOa€7jvOZöSDZK2W"
    "C9Kß9iABnÄavU€H3HT7I2wqJq9hÖB9U30zä3HumUDmjff2dYD€AnpR9FUÖPd7sa23äNE"
    "z0s3gNclyfd1kEjLpmÖ9wOölaghÄÜVBLPTwVoKbY2eUöU3F9jP0Fx62Jö€GgZ7RIUhxd"
    "3K1N1PTDMgHV8Bunqf9wjxOczzKnSbqEMmÜv1T€wäzl11GjÜXaH6JeccPYqqZGUt450O"
    "STeMapHü72gMßOMG4OIBuY3ozRTlFCäfpÄ3pzFlAprhyÜTÖÄCGL8zwe€ß€pNuj4VsQ€3"
    "ppmQTtß4gö2INvLöR1ÖZTnuhwh2197Mge€W9xTWEZSguüRAYBySoMöwcng3qüywmCnp0"
    "aämZXEcSeCLRül5HrjVFCHVJÜmVBzfjLkvs80GIÖdn1ÖyORQ€upGü€Äöq5w1€FucüZ€7"
    "nNÜC00MtIl9MSgaZkkUNÄkwbPRpWKuXX7Wu1Q6ÜfT4x6XIÖzdüJLitwxj6R7MSX6Cw6Q"
    "Da8qÖneW7cd€jZQraö1j1AkHn8H1mvz1wpÜuPqkEEoJOhhÜhb0CßvNDIDer1A2K8mVw1"
    "fHnK78sßÄrplm3hHyXjü";
    check_simple(s1);

    const char *s2 = "ÜßtWyÖZorBSö0lÄ€Cth7nFInQ55ygI€aEx8ßwQkNlßHähQÄV9eLqKnEumDSsäXFJcvSKf2fqäYvmxs8AJBRbOdvöShmLÖ3ont6Xy3ÄPWV€8mlYN6tmbbBIübPuMOfö1K1S9MPGCtyEFVlmJQyKSZaZ";
    check_simple(s2);

}

void check_random(size_t max_size) {
    char *s = generate_random_string(utf8_charset, utf8_charset_size, max_size);
    TEST_CASE(s);

    str8 str = str8new(s);
    size_t size = str8_size_simd(s, 0);
    size_t length = str8_count_chars_simd(s, size);
    uint8_t type = type_from_capacity(size);

    TEST_CHECK(str);
    TEST_CHECK_EQUAL(STR8_TYPE(str), type, "%d", "type");
    TEST_CHECK_EQUAL(str8size(str), size, "%zu", "size");
    TEST_CHECK_EQUAL(str8len(str), length, "%zu", "length");
    TEST_CHECK_EQUAL(str8cap(str), size, "%zu", "capacity");

    if (type > STR8_TYPE1 && str[-1] & 0x80) {
        void *list = checkpoints_list_ptr(str);
        TEST_CHECK(list);
        TEST_MSG("Expected list not to be NULL.");
        size_t value = read_entry(list, 0);
        TEST_CHECK(value <= CHECKPOINTS_GRANULARITY);
        TEST_MSG("Expected the first entry to be <= %d but got %zu", CHECKPOINTS_GRANULARITY, value);

        size_t list_size = size / CHECKPOINTS_GRANULARITY;
        if (list_size) {
            size_t idx = rand() % list_size;
            size_t byte_offset = (idx + 1) * CHECKPOINTS_GRANULARITY;

            value = read_entry(list, idx);
            size_t char_count_to_idx = str8_count_chars_simd(s, byte_offset);
            TEST_CHECK_EQUAL(value, char_count_to_idx, "%zu", "value");
        }
    }
    str8free(str);
    free(s);
}

void test_new_random_short(void) {
    srand(time(NULL));
    size_t max_size = 1000;

    for (int i=0; i<10000; i++) {
        check_random(rand()%max_size);
    }
}

void test_new_random_medium(void) {
    srand(time(NULL));
    size_t min_size = 10000;
    size_t max_size = 1000000;

    for (int i=0; i<100; i++) {
        check_random(rand()%(max_size-min_size)+min_size);
    }
}

void test_new_random_long(void) {
    srand(time(NULL));
    size_t min_size = 10000000;
    size_t max_size = 20000000; // Keep it reasonable to avoid excessive memory usage

    for (int i=0; i<2; i++) {
        size_t size = rand()%(max_size-min_size)+min_size;
        char *s = malloc(size + 1);
        TEST_ASSERT(s != NULL);
        memset(s, 'A', size);
        memcpy(s + 5000, "€€€", 9); // Add some UTF-8 chars
        s[size] = '\0';
        check_simple(s);
        free(s);
    }
}

void test_grow(void) {
    const char *s = "TEST";
    str8 str = str8new(s);
    str = str8grow(str, 20, false);
    TEST_CHECK(strcmp(str, "TEST") == 0);
    TEST_CHECK_EQUAL(str8size(str), 4LU, "%zu", "size");
    TEST_CHECK_EQUAL(str8cap(str), 20LU, "%zu", "capacity");
    
    str = str8grow(str, 100, true);
    TEST_CHECK_EQUAL(str8size(str), 4LU, "%zu", "size");
    TEST_CHECK_EQUAL(str8cap(str), 100LU, "%zu", "capacity");

    str = str8grow(str, 10000, false);
    TEST_CHECK_EQUAL(str8size(str), 4LU, "%zu", "size");
    TEST_CHECK_EQUAL(str8cap(str), 10000LU, "%zu", "capacity");
    TEST_CHECK_EQUAL(STR8_IS_ASCII(str), false, "%d", "ASCIII");
    str8free(str);
}



void test_append(void) {
    TEST_CASE("Type 0 to Type 0 (both ASCII)");
    {
        str8 str = str8new("TEST");
        str = str8append(str, "FOO");
        TEST_CHECK_EQUAL(str8size(str), 7LU, "%zu", "size");
        TEST_CHECK_EQUAL(str8cap(str), 10LU, "%zu", "capacity");
        TEST_CHECK_EQUAL(STR8_TYPE(str), STR8_TYPE1, "%d", "type");
        TEST_CHECK(strcmp(str, "TESTFOO") == 0);
        str8free(str);
    }
    TEST_CASE("Type1 to Type0 (both ASCII)");
    {
        const char *str1 = "TEST";
        const char *str2 = "FOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO";
        str8 str = str8new(str1);
        str = str8append(str, str2);
        TEST_CHECK_EQUAL(str8size(str), strlen(str1) + strlen(str2), "%zu", "size");
        TEST_CHECK_EQUAL(STR8_TYPE(str), STR8_TYPE1, "%d", "type");
        str8free(str);
    }

    TEST_CASE("Type1 to Type1 (both ASCII)");
    {
        const char *str1 = "TESTsdaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
        const char *str2 = "FOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO";
        str8 str = str8new(str1);
        str = str8append(str, str2);
        TEST_CHECK_EQUAL(str8size(str), strlen(str1) + strlen(str2), "%zu", "size");
        TEST_CHECK_EQUAL(STR8_TYPE(str), STR8_TYPE1, "%d", "type");
        str8free(str);
    }



    TEST_CASE("Type0 to Type0 (ASCII)");
    {
        const char *str1 = "TEST";
        const char *str2 = "FO€OOOO";
        str8 str = str8new(str1);
        str = str8append(str, str2);
        TEST_CHECK_EQUAL(str8size(str), strlen(str1) + strlen(str2), "%zu", "size");
        TEST_CHECK_EQUAL(STR8_TYPE(str), STR8_TYPE1, "%d", "type");
        TEST_CHECK_EQUAL(str8len(str), strlen(str1) + strlen(str2) - 2, "%zu", "length");
        str8free(str);
    }
    TEST_CASE("Type1 to Type0 (ASCII)");
    {
        const char *str1 = "TEST";
        const char *str2 = "FO€OOasjdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddOO";
        str8 str = str8new(str1);
        str = str8append(str, str2);
        TEST_CHECK_EQUAL(str8size(str), strlen(str1) + strlen(str2), "%zu", "size");
        TEST_CHECK_EQUAL(STR8_TYPE(str), STR8_TYPE1, "%d", "type");
        TEST_CHECK_EQUAL(str8len(str), strlen(str1) + strlen(str2) - 2, "%zu", "length");
        str8free(str);
    }
    TEST_CASE("Type1 to Type1 -> Type 2");
    {
        char *str1 = malloc(201);
        memset(str1, 'A', 200);
        str1[200] = '\0';
        char *str2 = malloc(501);
        memset(str2, 'B', 500);
        memcpy(str2, "€", 3);
        str2[500] = '\0';
        str8 str = str8new(str1);
        str = str8append(str, str2);
        TEST_CHECK_EQUAL(str8size(str), strlen(str1) + strlen(str2), "%zu", "size");
        TEST_CHECK_EQUAL(STR8_TYPE(str), STR8_TYPE2, "%d", "type");
        void *list = checkpoints_list_ptr(str);
        TEST_CHECK(list);
        TEST_MSG("Expected list not to be NULL.");
        TEST_CHECK_EQUAL(read_entry(list, 0), 510LU, "%zu", "value");
        str8free(str);
        free(str1);
        free(str2);
    }

    TEST_CASE("Type1 to Type2 (ASCII) -> Type 2");
    {
        char *str1 = malloc(1501);
        memset(str1, 'A', 1500);
        str1[1500] = '\0';
        char *str2 = malloc(501);
        memset(str2, 'B', 500);
        memcpy(str2, "€", 3);
        str2[500] = '\0';
        str8 str = str8new(str1);
        str = str8append(str, str2);
        TEST_CHECK_EQUAL(str8size(str), strlen(str1) + strlen(str2), "%zu", "size");
        TEST_CHECK_EQUAL(STR8_TYPE(str), STR8_TYPE2, "%d", "type");
        void *list = checkpoints_list_ptr(str);
        TEST_CHECK(list);
        TEST_MSG("Expected list not to be NULL.");
        TEST_CHECK_EQUAL(read_entry(list, 0), 512LU, "%zu", "value");
        str8free(str);
        free(str1);
        free(str2);
    }

    TEST_CASE("Type1 to Type2 -> Type 2");
    {
        char *str1 = malloc(1501);
        memset(str1, 'A', 1500);
        memcpy(str1, "€", 3);
        str1[1500] = '\0';
        char *str2 = malloc(501);
        memset(str2, 'B', 500);
        memcpy(str2, "€", 3);
        str2[500] = '\0';
        str8 str = str8new(str1);
        str = str8append(str, str2);
        TEST_CHECK_EQUAL(str8size(str), strlen(str1) + strlen(str2), "%zu", "size");
        TEST_CHECK_EQUAL(STR8_TYPE(str), STR8_TYPE2, "%d", "type");
        void *list = checkpoints_list_ptr(str);
        TEST_CHECK(list);
        TEST_MSG("Expected list not to be NULL.");
        TEST_CHECK_EQUAL(read_entry(list, 0), 510LU, "%zu", "value");
        TEST_CHECK_EQUAL(read_entry(list, 2), 1532LU, "%zu", "value");
        str8free(str);
        free(str1);
        free(str2);
    }
}

TEST_LIST = {
    { "New (simple)", test_new_simple },
    { "New (failed random tests)", test_failed_ranom_tests },
    { "New (random short)", test_new_random_short },
    { "New (random medium)", test_new_random_medium },
    { "New (random long)", test_new_random_long },
    { "Grow", test_grow },
    { "Append", test_append },
    { NULL, NULL }
};
