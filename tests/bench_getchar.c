#include "test_helper.h"
#include "bench_helper.h"
#include "src/str8.h"
#include "src/str8_header.h"

#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#ifndef CHECKPOINTS_GRANULARITY
#define CHECKPOINTS_GRANULARITY 512
#endif


// A volatile sink variable to ensure the compiler does not optimize away
// the function calls whose results we are not using directly.
volatile const char* sink;

int main(void) {
    srand(time(NULL));

    // --- 1. Setup ---
    const size_t STRING_SIZE = 5 * 1024 * 1024; // 5 MB string
    const int NUM_LOOKUPS = 20000;

    printf("Benchmark for str8getchar with CHECKPOINTS_GRANULARITY = %d\n", CHECKPOINTS_GRANULARITY);
    printf("------------------------------------------------------------\n");

    printf("Generating a %zu MB UTF-8 string...\n", STRING_SIZE / (1024 * 1024));
    char* raw_string = generate_random_string(utf8_charset, utf8_charset_size, STRING_SIZE);
    if (!raw_string) {
        fprintf(stderr, "Failed to allocate memory for the string.\n");
        return 1;
    }

    printf("Creating str8 object (this includes building the checkpoint list)...");
    str8 str = str8new(raw_string);
    if (!str) {
        fprintf(stderr, "Failed to create str8 object.\n");
        free(raw_string);
        return 1;
    }
    size_t len = str8len(str);
    printf("String created with %zu bytes and %zu characters.\n\n", str8size(str), len);


    // --- 2. Benchmark: Random Access Lookups ---
    // Pre-generate an array of random indices. This ensures that the `rand()`
    // function call is not part of the time measurement.
    size_t* lookups = malloc(NUM_LOOKUPS * sizeof(size_t));
    if (!lookups) {
        fprintf(stderr, "Failed to allocate memory for lookups.\n");
        str8free(str);
        free(raw_string);
        return 1;
    }
    for (int i = 0; i < NUM_LOOKUPS; i++) {
        lookups[i] = rand() % len;
    }

    printf("Performing %d random character lookups...\n", NUM_LOOKUPS);
    
    double time_random_us = MEASURE_TIME({
        for (int i = 0; i < NUM_LOOKUPS; i++) {
            sink = str8getchar(str, lookups[i]);
        }
    });

    printf("--- Results: Random Access ---\n");
    printf("  Total lookups: %d\n", NUM_LOOKUPS);
    printf("  Total time:    %.4f ms\n", time_random_us / 1000.0);
    printf("  Avg latency:   %.2f ns/lookup\n", (time_random_us * 1000.0) / NUM_LOOKUPS);
    putc('\n', stdout);


    // --- 3. Benchmark: Sequential Access Lookups (for comparison) ---
    printf("Performing %d sequential character lookups...\n", NUM_LOOKUPS);

    double time_sequential_us = MEASURE_TIME({
        for (int i = 0; i < NUM_LOOKUPS; i++) {
            sink = str8getchar(str, i);
        }
    });

    printf("--- Results: Sequential Access ---\n");
    printf("  Total lookups: %d\n", NUM_LOOKUPS);
    printf("  Total time:    %.4f ms\n", time_sequential_us / 1000.0);
    printf("  Avg latency:   %.2f ns/lookup\n", (time_sequential_us * 1000.0) / NUM_LOOKUPS);
    putc('\n', stdout);


    // --- 4. Cleanup ---
    free(lookups);
    str8free(str);
    free(raw_string);

    return 0;
}
