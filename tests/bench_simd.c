#include "test_helper.h"
#include "bench_helper.h"

#include "src/str8_simd.h"

size_t count_chars_scalar(const char *s, size_t size) {
    size_t char_count = 0;
    for (size_t i=0; i<size; i++) {
        if (((unsigned char)s[i] & 0xC0) != 0x80) {
            char_count++;
        }
    }
    return char_count;
}

#define BENCH_COUNT 1000

volatile size_t sink;


int main(void) {

    size_t max_strlen = 1000000;

    char *strings[BENCH_COUNT] = { 0 };
    for (int i=0; i<BENCH_COUNT; i++) {
        strings[i] = generate_random_string(utf8_charset, utf8_charset_size, rand() % max_strlen);
    }

    BENCH_DECLARE(count_chars_scalar);
    for (int i=0; i<BENCH_COUNT; i++) {
        char *s = strings[i];
        size_t size = str8_size_simd(s, max_strlen);
        double t = MEASURE_TIME({
            sink = count_chars_scalar(s, size);
        });
        BENCH_UPDATE(count_chars_scalar, t, size);
    }
    BENCH_PRINT_RESULTS(count_chars_scalar, BENCH_COUNT);

    putc('\n', stdout);

    BENCH_DECLARE(str8_count_chars_simd);
    for (int i=0; i<BENCH_COUNT; i++) {
        char *s = strings[i];
        size_t size = str8_size_simd(s, max_strlen);
        double t = MEASURE_TIME({
            sink = str8_count_chars_simd(s, size);
        });
        BENCH_UPDATE(str8_count_chars_simd, t, size);
    }
    BENCH_PRINT_RESULTS(str8_count_chars_simd, BENCH_COUNT);

    putc('\n', stdout);

    BENCH_DECLARE(strlen);
    for (int i=0; i<BENCH_COUNT; i++) {
        char *s = strings[i];
        size_t size = str8_size_simd(s, max_strlen);
        double t = MEASURE_TIME({
            sink = strlen(s);
        });
        BENCH_UPDATE(strlen, t, size);
    }
    BENCH_PRINT_RESULTS(strlen, BENCH_COUNT);


    putc('\n', stdout);

    BENCH_DECLARE(simd_size);
    for (int i=0; i<BENCH_COUNT; i++) {
        char *s = strings[i];
        size_t size = str8_size_simd(s, max_strlen);
        double t = MEASURE_TIME({
            sink = str8_size_simd(s, 0);
        });
        BENCH_UPDATE(simd_size, t, size);
    }
    BENCH_PRINT_RESULTS(simd_size, BENCH_COUNT);

    putc('\n', stdout);

    BENCH_DECLARE(simd_scan);
    for (int i=0; i<BENCH_COUNT; i++) {
        char *s = strings[i];
        size_t size = str8_size_simd(s, max_strlen);
        size_t first_non_ascii_pos = (size_t)-1;
        double t = MEASURE_TIME({
            sink = str8_scan_simd(s, 0, &first_non_ascii_pos);
        });
        BENCH_UPDATE(simd_scan, t, size);
    }
    BENCH_PRINT_RESULTS(simd_scan, BENCH_COUNT);

    for (int i=0; i<BENCH_COUNT; i++) {
        free(strings[i]);
    }
    
    return 0;
}