#include "acutest.h"
#include "test_helper.h"
#include "src/str8_header.h"
#include "src/str8_debug.h"

void test_type0(void) {
    TEST_CASE("Empty String");
    {
        char mem[2] = {
            0x00,  // type byte
            '\0'   // string
        };
        str8 str = &mem[1];
        TEST_CHECK_EQUAL(STR8_TYPE(str), 0, "%d", "type");
        TEST_CHECK_EQUAL(str8size(str), 0LU, "%zu", "size");
        TEST_CHECK_EQUAL(str8cap(str), 0LU, "%zu", "capacity");
        TEST_CHECK_EQUAL(str8len(str), 0LU, "%zu", "length");
    }
    TEST_CASE("ASCII String");
    {
        // size  type
        //     4   0
        // 00100 000 = 0x20
        char mem[] = "\x20TEST";
        str8 str = &mem[1];
        TEST_CHECK_EQUAL(STR8_TYPE(str), 0, "%d", "type");
        TEST_CHECK_EQUAL(str8size(str), 4LU, "%zu", "size");
        TEST_CHECK_EQUAL(str8cap(str), 4LU, "%zu", "capacity");
        TEST_CHECK_EQUAL(str8len(str), 4LU, "%zu", "length");
    }
    TEST_CASE("UTF-8 String");
    {
        // size  type
        //     9   0
        // 01001 000 = 0x48
        char mem[] = "\x48€€€";
        str8 str = &mem[1];
        TEST_CHECK_EQUAL(STR8_TYPE(str), 0, "%d", "type");
        TEST_CHECK_EQUAL(str8size(str), 9LU, "%zu", "size");
        TEST_CHECK_EQUAL(str8cap(str), 9LU, "%zu", "capacity");
        TEST_CHECK_EQUAL(str8len(str), 3LU, "%zu", "length");
    }
    TEST_CHECK("Set Properties");
    {
        char mem[] = "\x00";
        str8 str = &mem[1];

        // should set size to 8
        str8setsize(str, 8);
        TEST_CHECK_EQUAL(str8size(str), 8LU, "%zu", "size");
        TEST_CHECK_EQUAL(mem[0], (8 << 3), "%x", "first byte");

        // should set size to 31
        str8setsize(str, 31);
        TEST_CHECK_EQUAL(str8size(str), 31LU, "%zu", "size");
        TEST_CHECK_EQUAL((unsigned char)mem[0], (31U << 3), "%x", "first byte");

        // there is no length field in type 0, so nothing should change
        str8setsize(str, 0);
        str8setlen(str, 15);
        TEST_CHECK_EQUAL(str8len(str), 0LU, "%zu", "length");

        // type 0 capacity equals size. str8setcap() should do nothing
        str8setsize(str, 5);
        TEST_CHECK_EQUAL(str8size(str), 5LU, "%zu", "size");
        str8setcap(str, 15);
        TEST_CHECK_EQUAL(str8cap(str), 5LU, "%zu", "capacity");

    }
}

void test_field_size(void) {
    TEST_CHECK_EQUAL(STR8_FIELD_SIZE(STR8_TYPE1), 1, "%d", "type 1 field size");
    TEST_CHECK_EQUAL(STR8_FIELD_SIZE(STR8_TYPE2), 2, "%d", "type 2 field size");
    TEST_CHECK_EQUAL(STR8_FIELD_SIZE(STR8_TYPE4), 4, "%d", "type 4 field size");
    TEST_CHECK_EQUAL(STR8_FIELD_SIZE(STR8_TYPE8), 8, "%d", "type 8 field size");
}

void test_size_field(void) {
    TEST_CASE("Type 1");
    {
        char mem[3] = {
            0x00,  // size field
            0x01,  // type byte
            '\0'   // string
        };
        str8 str = &mem[2];
        TEST_CHECK_EQUAL(size_field(str, STR8_TYPE1), mem, "%p", "pointer");
    }
    TEST_CASE("Type 2");
    {
        char mem[] = {
            0x00,  // size field
            0x00,
            0x02,  // type byte
            '\0'   // string
        };
        str8 str = &mem[3];
        TEST_CHECK_EQUAL(size_field(str, STR8_TYPE2), mem, "%p", "pointer");
    }

    TEST_CASE("Type 8");
    {
        char mem[] = {
            0x00,  // size field
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x04,  // type byte
            '\0'   // string
        };
        str8 str = &mem[9];
        TEST_CHECK_EQUAL(size_field(str, STR8_TYPE8), mem, "%p", "pointer");
    }
}

void test_fields(void) {
    char mem[26] = { 0 };  // (max header size 8 * 3 + 1) + '\0' 
    str8 str = &mem[25];
    for (uint8_t type = 2; type <= 4; type++) {
        char descr[255];
        snprintf(descr, sizeof(descr), "Type %d", type);
        TEST_CASE(descr);
        
        ((uint8_t*)str)[-1] = type;

        TEST_CHECK_EQUAL(STR8_TYPE(str), type, "%d", "type");

        size_t max_val = type == STR8_TYPE2 ? UINT16_MAX : type == STR8_TYPE4 ? UINT32_MAX : UINT64_MAX;

        str8setsize(str, 0);
        str8setlen(str, 0);
        str8setcap(str, 0);
        TEST_CHECK_EQUAL(str8size(str), 0LU, "%zu", "size");
        TEST_CHECK_EQUAL(str8len(str), 0LU, "%zu", "length");
        TEST_CHECK_EQUAL(str8cap(str), 0LU, "%zu", "capacity");
        
        str8setsize(str, max_val);
        str8setlen(str, max_val);
        str8setcap(str, max_val);
        TEST_CHECK_EQUAL(str8size(str), max_val, "%zu", "size");
        TEST_CHECK_EQUAL(str8len(str), max_val, "%zu", "length");
        TEST_CHECK_EQUAL(str8cap(str), max_val, "%zu", "capacity");

        size_t some_val = 6781263871 % max_val;
        str8setsize(str, some_val);
        str8setlen(str, some_val);
        str8setcap(str, some_val);
        TEST_CHECK_EQUAL(str8size(str), some_val, "%zu", "size");
        TEST_CHECK_EQUAL(str8len(str), some_val, "%zu", "length");
        TEST_CHECK_EQUAL(str8cap(str), some_val, "%zu", "capacity");
    }
}

TEST_LIST = {
    { "Type 0", test_type0 },
    { "Field Size", test_field_size },
    { "Size Field", test_size_field },
    { "Fields", test_fields },
    { NULL, NULL }
};