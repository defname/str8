#include "acutest.h"
#include "test_helper.h"

#define CHECKPOINTS_GRANULARITY 512

#include "src/str8_checkpoints.h"
#include "src/str8_debug.h"
#include "src/str8_header.h"
#include "src/str8.h"
#include "src/str8_simd.h"


void test_checkpoints_entry_offset(void) {
    // idx 12: 12 * 2 Bytes
    TEST_CHECK_EQUAL(checkpoints_entry_offset(12), 24UL, "%zu", "bytes");

    // idx 500
    // max 2 bytes index:
    // max 2 bytes value = 65535
    // index 0 ---> 512
    // index 1 ---> 1024
    // index n ---> (n + 1) * 512
    // ...
    // index 126 ---> 65024  (next value overflows)
    //
    // >>> 126 * 2 Bytes = 252 Bytes
    //
    // index 127 ---> 65536
    // ...
    // index 500 ---> 256512
    //
    // >>> (500 - 126) * 4 Bytes = 1496 Bytes
    //
    // >>> 252 Bytes + 1496 Bytes = 1748 Bytes
    TEST_CHECK_EQUAL(checkpoints_entry_offset(500), 1748UL, "%zu", "bytes");

    // idx 8388608
    // idx           0 -       126 ---> 2 Bytes
    // idx         127 -   8388606 ---> 4 Bytes
    // idx     8388607 -   8388608 ---> 8 Bytes
    size_t bytes = 126 * 2 + (8388606 - 126) * 4 + 2 * 8;
    TEST_CHECK_EQUAL(checkpoints_entry_offset(8388608), bytes, "%zu", "bytes");

    // edge case 126, 127
    TEST_CHECK_EQUAL(checkpoints_entry_offset(126), 252UL, "%zu", "bytes");
    TEST_CHECK_EQUAL(checkpoints_entry_offset(127), 256UL, "%zu", "bytes");
}

void test_checkpoints_list(void) {
    // string with capacity 256100
    // 256100 / 512 == 500.1953125   --> 500 entries in the list
    //                               --> idx 0 - 499 in the list
    //                               --> idx 500 just after the last entry
    //                               --> 1748UL == checkpoints_entry_offset(500) == byte size of the list
    const size_t list_byte_size = 1748UL;
    const size_t header_size = 1 + 3 * 4;   // string type 4
    char mem[list_byte_size + header_size + 1];
    memset(mem, 0xFF, list_byte_size + header_size);
    str8 str = &mem[list_byte_size + header_size];
    str[0] = '\0';
    str[-1] = 0x80 | STR8_TYPE4;

    TEST_CHECK_EQUAL(STR8_TYPE(str), STR8_TYPE4, "%d", "type");
    TEST_CHECK_EQUAL(STR8_IS_ASCII(str), false, "%d", "ASCII");

    str8setlen(str, 0);
    str8setsize(str, 0);
    str8setcap(str, 256100);

    void *list_pointer = checkpoints_list(str);
    TEST_CHECK(list_pointer == mem);
    ptrdiff_t diff = (ptrdiff_t)list_pointer - (ptrdiff_t)mem;
    TEST_MSG("Expected list pointer to be %p, but got %p (%ld bytes difference)", mem, list_pointer, diff);
}

void test_analyze_1(void) {
    char input[1300];
    memset(input, 'A', 1300);
    input[1299] = '\0';

    uint16_t list[MAX_2BYTE_INDEX + 1];
    str8_analyze_config config = {
        .list = list,
        .list_capacity = MAX_2BYTE_INDEX + 1,
        .byte_offset = 0
    };
    str8_analyze_results results;
    int error = str8_analyze(input, 0, config, &results);

    TEST_CHECK_EQUAL(error, 0, "%d", "error");
    TEST_CHECK_EQUAL(results.list_size, 2LU, "%zu", "list size");
    TEST_CHECK_EQUAL(results.size, 1299LU, "%zu", "size");
    TEST_CHECK_EQUAL(results.length, 1299LU, "%zu", "length");
}

void test_analyze_2(void) {
    // with list reallocation
    // more than MAX_2BYTE_INDEX / CHECKPOINTS_GRANULARITY entries are needed
    // 66000 / 512 = 128.9xx = 128 entries
    char input[66001];
    memset(input, 'A', 66001);
    input[66000] = '\0';

    uint16_t list[MAX_2BYTE_INDEX + 1];
    str8_analyze_config config = {
        .list = list,
        .list_capacity = MAX_2BYTE_INDEX + 1,
        .byte_offset = 0
    };
    str8_analyze_results results;
    int error = str8_analyze(input, 0, config, &results);

    TEST_CHECK_EQUAL(error, 0, "%d", "error");
    TEST_CHECK_EQUAL(results.list_size, 128LU, "%zu", "list size");

    free(results.list);
}


void test_analyze_3(void) {
    // with list reallocation
    // test with input > 4GB
    char *input = malloc(4294967397);
    TEST_ASSERT(input);
    memset(input, 'A', 4294967397);
    input[4294967396] = '\0';

    uint16_t list[MAX_2BYTE_INDEX + 1];
    str8_analyze_config config = {
        .list = list,
        .list_capacity = MAX_2BYTE_INDEX + 1,
        .byte_offset = 0
    };
    str8_analyze_results results;
    int error = str8_analyze(input, 0, config, &results);

    TEST_CHECK_EQUAL(error, 0, "%d", "error");
    TEST_CHECK_EQUAL(results.list_size, 8388608LU, "%zu", "list size");
    TEST_CHECK_EQUAL(results.size, 4294967396LU, "%zu", "size");
    TEST_CHECK_EQUAL(results.length, 4294967396LU, "%zu", "size");
    TEST_CHECK_EQUAL(read_entry(results.list, 8388606LU), 512LU * 8388607LU, "%zu", "entry value");

    free(results.list);
    free(input);
}

void test_analyze_4(void) {
    // with list multiple  reallocation and UTF-8 characters
    char *input = malloc(123123);
    TEST_ASSERT(input);
    memset(input, 'A', 123123);
    memcpy(input + 1234, "€", 3);
    input[123122] = '\0';

    uint16_t list[MAX_2BYTE_INDEX + 1];
    str8_analyze_config config = {
        .list = list,
        .list_capacity = MAX_2BYTE_INDEX + 1,
        .byte_offset = 0
    };
    str8_analyze_results results;
    int error = str8_analyze(input, 0, config, &results);

    TEST_CHECK_EQUAL(error, 0, "%d", "error");
    // str size / 512
    TEST_CHECK_EQUAL(results.list_size, 240LU, "%zu", "list size");
    // str size
    TEST_CHECK_EQUAL(results.size, 123122LU, "%zu", "size");
    // str size - 2 byte (of the € sign)
    TEST_CHECK_EQUAL(results.length, 123122LU - 2LU, "%zu", "size");
    // (index + 1) * 512 - 2 bytes (€ sign)
    TEST_CHECK_EQUAL(read_entry(results.list, 230LU), 512LU * 231LU - 2LU, "%zu", "entry value");

    free(results.list);
    free(input);
}

void test_read_write(void) {
    // with list reallocation
    // more than MAX_2BYTE_INDEX / CHECKPOINTS_GRANULARITY entries are needed
    // 66000 / 512 = 128.9xx = 128 entries
    char input[66001];
    memset(input, 'A', 66001);
    input[66000] = '\0';

    uint16_t list[MAX_2BYTE_INDEX + 1];
    str8_analyze_config config = {
        .list = list,
        .list_capacity = MAX_2BYTE_INDEX + 1,
        .byte_offset = 0
    };
    str8_analyze_results results;
    int error = str8_analyze(input, 0, config, &results);

    TEST_CHECK_EQUAL(error, 0, "%d", "error");
    TEST_CHECK_EQUAL(results.list_size, 128LU, "%zu", "list size");

    for (size_t i=0; i<128LU; i++) {
        write_entry(results.list, i, i);
    }
    for (size_t i=0; i<128LU; i++) {
        TEST_CHECK_EQUAL(read_entry(results.list, i), i, "%zu", "entry value");
    }
    for (size_t i=0; i<128LU; i++) {
        write_entry(results.list, 128LU-i, i);
    }
    for (size_t i=0; i<128LU; i++) {
        TEST_CHECK_EQUAL(read_entry(results.list, 128LU-i), i, "%zu", "entry value");
    }

    free(results.list);
}

void test_find_entry_ub(void) {
    uint16_t list[100];
    for (size_t i=0; i<100; i++) {
        write_entry(list, i, (i+1)*100);
    }
    //  idx      0    1          99
    // list = {100, 200, ..., 10000}

    TEST_CHECK_EQUAL(find_entry_ub(list, 100, 550), 4LU, "%zu", "index");
    TEST_CHECK_EQUAL(find_entry_ub(list, 100, 0), 100LU, "%zu", "index");
    TEST_CHECK_EQUAL(find_entry_ub(list, 100, 99), 100LU, "%zu", "index");
    TEST_CHECK_EQUAL(find_entry_ub(list, 100, 100), 0LU, "%zu", "index");
    TEST_CHECK_EQUAL(find_entry_ub(list, 100, 199), 0LU, "%zu", "index");
    TEST_CHECK_EQUAL(find_entry_ub(list, 100, 200), 1LU, "%zu", "index");
    TEST_CHECK_EQUAL(find_entry_ub(list, 100, 100000000), 99LU, "%zu", "index");
}

void test_getchar(void) {
    TEST_CASE("Short string");
    {
        const char *s = "Foooobar";
        str8 str = str8new(s);
        TEST_CHECK_EQUAL(str8getchar(str, 0), str, "%s", "result");
        TEST_CHECK_EQUAL(str8getchar(str, 1), str+1, "%s", "result");
        TEST_CHECK_EQUAL(str8getchar(str, 6), str+6, "%s", "result");
        str8free(str);
    }
    TEST_CASE("Out-of-bound");
    {
        const char *s = "Foooobar";
        str8 str = str8new(s);
        TEST_CHECK_EQUAL(str8getchar(str, -1), NULL, "%p", "result");
        TEST_CHECK_EQUAL(str8getchar(str, 8), NULL, "%p", "result");
        TEST_CHECK_EQUAL(str8getchar(str, 10), NULL, "%p", "result");
        str8free(str);
    }
    TEST_CASE("UTF-8");
    {
        const char *s = "Fooo€obar";
        str8 str = str8new(s);
        TEST_CHECK_EQUAL(str8getchar(str, 0), str, "%s", "result");
        TEST_CHECK_EQUAL(str8getchar(str, 1), str+1, "%s", "result");
        TEST_CHECK_EQUAL(str8getchar(str, 4), str+4, "%s", "result");
        TEST_CHECK_EQUAL(str8getchar(str, 5), str+7, "%s", "result");
        str8free(str);
    }
    TEST_CASE("UTF-8 with list");
    {
        char s[10000] = { 0 };
        memset(s, 'A', 9999);
        memcpy(s + 100, "€", 3);
        str8 str = str8new(s);

        void *list = checkpoints_list_ptr(str);
        TEST_CHECK_EQUAL(read_entry(list, 0), 510LU, "%zu", "list entry");
        TEST_CHECK_EQUAL(read_entry(list, 1), 510LU + 512LU, "%zu", "list entry");
        TEST_CHECK_EQUAL(read_entry(list, 13), 510LU + 13 * 512LU, "%zu", "list entry");

        TEST_CHECK_EQUAL(str8getchar(str, 0), str, "%p", "result");
        TEST_CHECK_EQUAL(str8getchar(str, 1), str+1, "%s", "result");
        TEST_CHECK_EQUAL(str8getchar(str, 100), str+100, "%s", "result");
        TEST_CHECK_EQUAL(str8getchar(str, 101), str+103, "%s", "result");
        str8free(str);
    }
    TEST_CASE("UTF-8 with long list");
    {
        const size_t size = 65558;
        char s[size + 1];
        s[size] = '\0';
        memset(s, 'A', size);
        memcpy(s + 100, "€", 3);
        str8 str = str8new(s);

        void *list = checkpoints_list_ptr(str);
        TEST_CHECK_EQUAL(read_entry(list, 0), 510LU, "%zu", "list entry");
        TEST_CHECK_EQUAL(read_entry(list, 1), 510LU + 512LU, "%zu", "list entry");
        TEST_CHECK_EQUAL(read_entry(list, 13), 510LU + 13 * 512LU, "%zu", "list entry");

        TEST_CHECK_EQUAL(str8getchar(str, 0), str, "%p", "result");
        TEST_CHECK_EQUAL(str8getchar(str, 1), str+1, "%s", "result");
        TEST_CHECK_EQUAL(str8getchar(str, 100), str+100, "%s", "result");
        TEST_CHECK_EQUAL(str8getchar(str, 101), str+103, "%s", "result");
        TEST_CHECK_EQUAL(str8getchar(str, 510), str+512, "%s", "result");
        TEST_CHECK_EQUAL(str8getchar(str, 511), str+513, "%s", "result");

        TEST_CHECK_EQUAL(str8getchar(str, 65000), str+65002, "%s", "result");
        str8free(str);
    }
}

void check_getchar(const char *s, size_t idx) {
    str8 str = str8new(s);
    const char *result = str8getchar(str, idx);
    const char *check = str8_lookup_idx_simd(str, idx, str8size(str));
    str8free(str);
    TEST_CHECK_EQUAL(result, check, "%p", "result");
}

void test_getchar_random(void) {
    for (int i=0; i<300; i++) {
        size_t max_size = rand() % 1000000;
        char *s = generate_random_string(utf8_charset, utf8_charset_size, max_size);
        TEST_CASE(s);
        for (int j=0; j<10; j++) {
            size_t idx = rand() % (max_size + 1);
            check_getchar(s, idx);
        }
        free(s);
    }
}

#ifdef SKIP_LARGE_MEMORY_TESTS
void dummy(void) {
}
#endif

TEST_LIST = {
    { "Checkpoints Entry Offset", test_checkpoints_entry_offset },
    { "Checkpoints List Pointer", test_checkpoints_list },
    { "Analyze 1", test_analyze_1 },
    { "Analyze 2", test_analyze_2 },
#ifdef SKIP_LARGE_MEMORY_TESTS
    { "Analyze 3 -- skipped", dummy },
#else
    { "Analyze 3", test_analyze_3 },
#endif
    { "Analyze 4", test_analyze_4 },
    { "Read Write", test_read_write },
    { "Find Entry UB", test_find_entry_ub },
    { "Get Char", test_getchar },
    { "Get Char Random", test_getchar_random },
    { NULL, NULL }
};