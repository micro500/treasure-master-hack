#include "bench_harness.h"
#include "bench_select.h"
#include "test_vectors.h"
#include "rng_obj.h"
#include "tm_cpu_factory.h"
#include <cstring>

static RNG g_rng;

static void noop_progress(double) {}

template<typename T>
static void bench_pipeline(const char* name) {
    T impl(&g_rng, pipeline_v0_key);
    uint8 out[128];
    run_bench(name, [&]() {
        impl.test_bruteforce_data(pipeline_v0_data, out);
        DoNotOptimize(out);
    });
}

template<typename T>
static void bench_boinc(const char* name) {
    T impl(&g_rng, pipeline_v0_key);
    static uint8 result_buf[1 << 20];
    uint32 result_size = 0;
    const uint32 N = 100000;
    run_bench(name, N, [&]() {
        result_size = 0;
        impl.run_bruteforce_boinc(0, N, noop_progress, result_buf, sizeof(result_buf), &result_size);
        DoNotOptimize(result_size);
    }, 3, 5);
}

template<typename T>
static void bench_checksum(const char* name) {
    T impl(&g_rng, validate_carnival_key);
    run_bench(name, [&]() {
        bool result = impl.test_bruteforce_checksum(validate_carnival_data, CARNIVAL_WORLD);
        DoNotOptimize(result);
    });
}

template<typename T>
static void bench_alg(const char* name, int alg_id, const uint8_t* input, uint16_t seed_in) {
    T impl(&g_rng);
    uint8 data[128];
    const int N = 100000;
    run_bench(name, N, [&]() {
        memcpy(data, input, 128);
        uint16 seed = seed_in;
        impl.test_algorithm_n(alg_id, data, &seed, N);
        DoNotOptimize(data);
    });
}

#define BENCH(fn, T) fn<T>(#fn "/" #T)

int main() {
    printf("%-55s  %-27s  %-27s\n", "benchmark", "mean", "median");
    printf("----------------------------------------------------------------------------------------------------------\n");

    


    auto y = tm_cpu_factory::get_impls_for_isa(detect_isa());

	
    static uint8_t result_buf[1 << 20];
    uint32_t result_size = 0;
    int N = 10000;

    ImplType x = tm_cpu_factory::find_by_name("avx2_m256s_map_8").value();
	auto z = tm_cpu_factory::create(x, &g_rng, pipeline_v0_key);

    z->run_bruteforce_boinc(0, static_cast<uint32_t>(N), noop_progress, result_buf, sizeof(result_buf), &result_size);
    
    for (auto& z : y) {
        TM_base* impl = tm_cpu_factory::create(z, &g_rng, pipeline_v0_key);
        run_bench(impl->obj_name.c_str(), N, [&]() {
            result_size = 0;
            impl->run_bruteforce_boinc(0, static_cast<uint32_t>(N), noop_progress, result_buf, sizeof(result_buf), &result_size);
            DoNotOptimize(result_buf);
            }, 3, 5);
	}
    
    printf("\n");

    for (int alg = 0; alg <= 7; alg++) {
        const uint8_t* input;
        uint16_t seed;
        switch (alg) {
            case 0: input = alg0_v0_in; seed = alg0_v0_seed_in; break;
            case 1: input = alg1_v0_in; seed = alg1_v0_seed_in; break;
            case 2: input = alg2_v0_in; seed = alg2_v0_seed_in; break;
            case 3: input = alg3_v0_in; seed = alg3_v0_seed_in; break;
            case 4: input = alg4_v0_in; seed = alg4_v0_seed_in; break;
            case 5: input = alg5_v0_in; seed = alg5_v0_seed_in; break;
            case 6: input = alg6_v0_in; seed = alg6_v0_seed_in; break;
            case 7: input = alg7_v0_in; seed = alg7_v0_seed_in; break;
            default: continue;
        }

        for (auto& z : y) {


            TM_base* impl = tm_cpu_factory::create(z, &g_rng, pipeline_v0_key);
            uint8_t data[128];
            N = 100000;
            char name[64];
            snprintf(name, sizeof(name), "bench_alg(%d)/%s", alg, impl->obj_name.c_str());

            run_bench(name, 100000, [&]() {
                memcpy(data, input, 128);
                uint16_t seed = 0;
                impl->test_algorithm_n(alg, data, &seed, N);
                DoNotOptimize(data);
                });
        }
        printf("\n");
    }
}
