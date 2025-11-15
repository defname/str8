#include "test_helper.h"
#include "bench_helper.h"
#include <string.h>

#include "src/str8_simd.h"

#define BENCH_COUNT 1000

// Use a volatile sink to prevent the compiler from optimizing away results.
volatile size_t sink_size;
volatile bool sink_bool;

int main(void) {

    size_t max_strlen = 1000000;

    char **strings = malloc(BENCH_COUNT * sizeof(char*));
    size_t *sizes = malloc(BENCH_COUNT * sizeof(size_t));

    for (int i=0; i<BENCH_COUNT; i++) {
        strings[i] = generate_random_string(utf8_charset, utf8_charset_size, rand() % max_strlen);
        sizes[i] = strlen(strings[i]);
    }

    // --- Benchmark: count_chars ---
    BENCH_DECLARE(count_chars_simd);
    for (int i=0; i<BENCH_COUNT; i++) {
        char *s = strings[i];
        size_t size = sizes[i];
        double t = MEASURE_TIME({
            sink_size = count_chars(s, size);
        });
        BENCH_UPDATE(count_chars_simd, t, size);
    }
    BENCH_PRINT_RESULTS(count_chars_simd, BENCH_COUNT);

    putc('\n', stdout);

    // --- Benchmark: is_ascii ---
    BENCH_DECLARE(is_ascii_simd);
    for (int i=0; i<BENCH_COUNT; i++) {
        char *s = strings[i];
        size_t size = sizes[i];
        double t = MEASURE_TIME({
            sink_bool = is_ascii(s, size);
        });
        BENCH_UPDATE(is_ascii_simd, t, size);
    }
    BENCH_PRINT_RESULTS(is_ascii_simd, BENCH_COUNT);


    for (int i=0; i<BENCH_COUNT; i++) {
        free(strings[i]);
    }
    free(strings);
    free(sizes);
    
    return 0;
}