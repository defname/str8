#ifndef BENCH_HELPER_H
#define BENCH_HELPER_H

#include <time.h>  // timespec, clock_gettime()
#include <stdio.h>  // printf()
#include <float.h>  // DBL_MAX
#include <stdint.h> // For size_t

/** @brief Return current time in microseconds (us) */
static inline double now() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000000.0 + (double)ts.tv_nsec / 1000.0;
}

/**
 * @brief Measures the execution time of a code block and returns the result in us.
 * @note This uses a GCC/Clang specific extension (Statement Expressions).
 */
#define MEASURE_TIME(...) ({ \
    double start_time_ = now(); \
    __VA_ARGS__; \
    double end_time_ = now(); \
    end_time_ - start_time_; \
})


// --- Benchmark Macro Toolkit ---

/**
 * @brief Declares variables for a benchmark, tracking normalized per-byte efficiency.
 */
#define BENCH_DECLARE(prefix) \
    double prefix##_min_ns_per_byte = DBL_MAX; \
    double prefix##_max_ns_per_byte = 0.0; \
    double prefix##_sum_time = 0.0; \
    size_t prefix##_total_workload = 0

/**
 * @brief Updates benchmark stats with time and workload, calculating ns/byte efficiency.
 */
#define BENCH_UPDATE(prefix, time_us, workload) do { \
    prefix##_sum_time += (time_us); \
    prefix##_total_workload += (workload); \
    if ((workload) > 0) { \
        double ns_per_byte = ((time_us) * 1000.0) / (double)(workload); \
        if (ns_per_byte < prefix##_min_ns_per_byte) prefix##_min_ns_per_byte = ns_per_byte; \
        if (ns_per_byte > prefix##_max_ns_per_byte) prefix##_max_ns_per_byte = ns_per_byte; \
    } \
} while (0)

/**
 * @brief Calculates and prints the final benchmark results, including throughput and efficiency.
 */
#define BENCH_PRINT_RESULTS(prefix, count) do { \
    double sum_time_val = prefix##_sum_time; \
    size_t total_workload_val = prefix##_total_workload; \
    double avg_time_us = sum_time_val / (double)(count); \
    double total_time_sec = sum_time_val / 1000000.0; \
    double throughput_gb_s = (total_time_sec > 0) \
        ? ((double)total_workload_val / (1024.0 * 1024.0 * 1024.0)) / total_time_sec \
        : 0; \
    double avg_workload_bytes = (count > 0) ? (double)total_workload_val / (double)(count) : 0; \
    printf("--- Benchmark: %s ---\n", #prefix); \
    printf("  Count:        %d\n", (int)(count)); \
    printf("  Avg Workload: %8.2f B\n", avg_workload_bytes); \
    printf("  Avg Time:     %8.4f us\n", avg_time_us); \
    printf("  Best (ns/B):  %8.4f\n", prefix##_min_ns_per_byte); \
    printf("  Worst (ns/B): %8.4f\n", prefix##_max_ns_per_byte); \
    printf("  Throughput:   %8.2f GB/s\n", throughput_gb_s); \
} while (0)


#endif // BENCH_HELPER_H