// WASM_IMPL_CLASS and its header are injected at compile time via:
//   -DWASM_IMPL_CLASS=<class_name>  -include ../cpu/<class_name>.h
#include <emscripten/emscripten.h>
#include <memory>

#include "rng_obj.h"
#include "key_schedule.h"

static RNG g_rng;
static std::unique_ptr<WASM_IMPL_CLASS> g_impl;

static void noop_progress(double) {}

extern "C" {

EMSCRIPTEN_KEEPALIVE
void tm_wasm_init(uint32_t key) {
    g_impl = std::make_unique<WASM_IMPL_CLASS>(&g_rng, key);
}

EMSCRIPTEN_KEEPALIVE
uint32_t tm_wasm_run(uint32_t start_data, uint32_t count, uint8_t* out_buf, uint32_t out_buf_size) {
    uint32_t result_size = 0;
    g_impl->run_bruteforce_boinc(start_data, count, noop_progress, out_buf, out_buf_size, &result_size);
    return result_size;
}

EMSCRIPTEN_KEEPALIVE
void tm_wasm_compute_flags(uint32_t data, uint8_t* carnival_out, uint8_t* other_out) {
    g_impl->compute_challenge_flags(data, *carnival_out, *other_out);
}

EMSCRIPTEN_KEEPALIVE
void tm_wasm_test_expansion(uint32_t data, uint8_t* result_128) {
    g_impl->test_expansion(data, result_128);
}

EMSCRIPTEN_KEEPALIVE
void tm_wasm_test_bruteforce_data(uint32_t data, uint8_t* result_128) {
    g_impl->test_bruteforce_data(data, result_128);
}

EMSCRIPTEN_KEEPALIVE
int tm_wasm_test_bruteforce_checksum(uint32_t data, int world) {
    return g_impl->test_bruteforce_checksum(data, world) ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE
void tm_wasm_test_algorithm(int algorithm_id, uint8_t* data_128, uint16_t* rng_seed_inout) {
    g_impl->test_algorithm(algorithm_id, data_128, rng_seed_inout);
}

EMSCRIPTEN_KEEPALIVE
void tm_wasm_test_algorithm_chain(const uint8_t* algorithm_ids, int chain_length,
                                  uint8_t* data_128, uint16_t* rng_seed_inout) {
    g_impl->test_algorithm_chain(algorithm_ids, chain_length, data_128, rng_seed_inout);
}

EMSCRIPTEN_KEEPALIVE
int tm_wasm_tracks_rng_state(void) {
    return g_impl->tracks_rng_state() ? 1 : 0;
}

} // extern "C"
