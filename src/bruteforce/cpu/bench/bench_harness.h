#pragma once
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <numeric>

#ifdef _MSC_VER
#include <intrin.h>
#endif

template<typename T>
inline void DoNotOptimize(T& val) {
#ifdef _MSC_VER
    _ReadWriteBarrier();
    (void)val;
#else
    asm volatile("" : "+r,m"(val) : : "memory");
#endif
}

static void print_bench(const char* name, double mean_ns, double median_ns) {
    double mean_mps   = 1e9 / mean_ns   / 1e6;
    double median_mps = 1e9 / median_ns / 1e6;
    printf("%-55s  mean: %8.1f ns %8.2f M/s  median: %8.1f ns %8.2f M/s\n",
        name, mean_ns, mean_mps, median_ns, median_mps);
}

struct BenchResult {
    double mean_ns;
    double median_ns;
    double mean_mops;
    double median_mops;
};

template<typename Fn>
static BenchResult measure_bench(const char* name, long long count_per_call, Fn fn,
                                 int warmup, int probe_count, int n_samples,
                                 double target_ns = 5e8) {
    for (int i = 0; i < warmup; i++) fn();

    auto t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < probe_count; i++) fn();
    auto t1 = std::chrono::high_resolution_clock::now();
    double probe_ns = std::chrono::duration<double, std::nano>(t1 - t0).count();

    long long iters = (long long)(target_ns / (probe_ns / probe_count));
    if (iters < 1) iters = 1;

    std::vector<double> samples;
    samples.reserve(n_samples);

    for (int s = 0; s < n_samples; s++) {
        auto start = std::chrono::high_resolution_clock::now();
        for (long long i = 0; i < iters; i++) fn();
        auto end = std::chrono::high_resolution_clock::now();
        double total_ns = std::chrono::duration<double, std::nano>(end - start).count();
        double scale = static_cast<double>(iters * count_per_call);
        samples.push_back(total_ns / scale);
    }

    double mean = std::accumulate(samples.begin(), samples.end(), 0.0) / n_samples;
    std::sort(samples.begin(), samples.end());
    double median_ns = samples[n_samples / 2];

    (void)name;
    return BenchResult{ mean, median_ns, 1e9 / mean / 1e6, 1e9 / median_ns / 1e6 };
}

template<typename Fn>
static void run_bench(const char* name, long long count_per_call, Fn fn,
                      int warmup = 100, int probe_count = 1000) {
    BenchResult r = measure_bench(name, count_per_call, fn, warmup, probe_count, 11);
    print_bench(name, r.mean_ns, r.median_ns);
}

template<typename Fn>
static void run_bench(const char* name, Fn fn) {
    BenchResult r = measure_bench(name, 1, fn, 100, 1000, 11);
    print_bench(name, r.mean_ns, r.median_ns);
}
