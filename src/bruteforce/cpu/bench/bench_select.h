#pragma once
#include "bench_harness.h"
#include "tm_cpu_factory.h"
#include "rng_obj.h"
#include <memory>
#include <string>
#include <vector>

#ifdef _MSC_VER
#include <intrin.h>
#endif

static ISA detect_isa() {
    ISA isa = ISA::X86_64;

    int info[4];
    __cpuid(info, 0);
    int max_leaf = info[0];

    if (max_leaf >= 1) {
        __cpuid(info, 1);
        bool has_ssse3 = (info[2] >> 9) & 1;
        bool has_avx   = (info[2] >> 28) & 1;
        bool has_osxsave = (info[2] >> 27) & 1;

        if (has_ssse3) isa = isa | ISA::SSSE3;

        if (has_avx && has_osxsave) {
            isa = isa | ISA::AVX;
        }
    }

    if (max_leaf >= 7) {
        __cpuidex(info, 7, 0);
        bool has_avx2      = (info[1] >> 5)  & 1;
        bool has_avx512bw  = (info[1] >> 30) & 1;
        bool has_avx512vl  = (info[1] >> 31) & 1;

        if (has_avx2)     isa = isa | ISA::AVX2;
        if (has_avx512bw) isa = isa | ISA::AVX512BW;
        if (has_avx512vl) isa = isa | ISA::AVX512VL;
    }

    return isa;
}

struct BenchBoincResult {
    ImplType    impl_type;
    std::string name;
    double      median_ns;
};

static std::vector<BenchBoincResult> benchmark_boinc_impls(
    RNG* rng, uint32_t key, ISA isa,
    int warmup = 2, int samples = 5)
{
    auto impls = tm_cpu_factory::get_impls_for_isa(isa);

    static uint8_t result_buf[1 << 20];
    const int N = 10000;
    auto noop = [](double) {};

    std::vector<BenchBoincResult> results;
    results.reserve(impls.size());

    for (auto type : impls) {
        std::unique_ptr<TM_base> impl(tm_cpu_factory::create(type, rng, key));
        uint32_t result_size = 0;
        BenchResult r = measure_bench(
            impl->obj_name.c_str(), N,
            [&]() {
                result_size = 0;
                impl->run_bruteforce_boinc(0, static_cast<uint32_t>(N), noop,
                                           result_buf, sizeof(result_buf), &result_size);
            },
            warmup, 1, samples, 1e8);
        results.push_back({ type, impl->obj_name, r.median_ns });
    }

    std::sort(results.begin(), results.end(),
              [](const BenchBoincResult& a, const BenchBoincResult& b) {
                  return a.median_ns < b.median_ns;
              });

    return results;
}
