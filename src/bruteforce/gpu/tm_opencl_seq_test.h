#pragma once
#include <memory>
#include <optional>
#include <vector>
#include <cstring>
#include "opencl.h"
#include "tm_opencl_seq.h"
#include "tm_base.h"
#include "key_schedule.h"
#include "rng_obj.h"

// Wraps tm_opencl_seq with the same test interface as CPU implementations,
// allowing tm_opencl_seq to participate in the smoke_test TYPED_TEST_SUITE.
//
// The opencl context and tm_opencl_seq instance are shared statics so they
// are created only once regardless of how many test instances are constructed.
class tm_opencl_seq_test
{
public:
    tm_opencl_seq_test(RNG* rng) : _rng(rng), _key(0)
    {
        _ensure_gpu(rng);
    }

    tm_opencl_seq_test(RNG* rng, uint32_t key) : _rng(rng), _key(key)
    {
        _ensure_gpu(rng);
        _schedule = key_schedule(key, key_schedule::ALL_MAPS);
        _build_schedule_flat();
    }

    void test_algorithm(int algorithm_id, uint8_t* data, uint16* rng_seed)
    {
        uint8_t output[128];
        uint8_t alg = (uint8_t)algorithm_id;
        _s_gpu->test_alg_batch(&alg, rng_seed, data, output, rng_seed, 1);
        memcpy(data, output, 128);
    }

    void test_expansion(uint32_t data_val, uint8_t* result_out)
    {
        uint8_t key_bytes[4]  = { (uint8_t)(_key >> 24), (uint8_t)(_key >> 16), (uint8_t)(_key >> 8), (uint8_t)_key };
        uint8_t data_bytes[4] = { (uint8_t)(data_val >> 24), (uint8_t)(data_val >> 16), (uint8_t)(data_val >> 8), (uint8_t)data_val };
        _s_gpu->test_expand_batch(key_bytes, data_bytes, result_out, 1);
    }

    void test_bruteforce_data(uint32_t data_val, uint8_t* result_out)
    {
        uint8_t key_bytes[4]  = { (uint8_t)(_key >> 24), (uint8_t)(_key >> 16), (uint8_t)(_key >> 8), (uint8_t)_key };
        uint8_t data_bytes[4] = { (uint8_t)(data_val >> 24), (uint8_t)(data_val >> 16), (uint8_t)(data_val >> 8), (uint8_t)data_val };
        _s_gpu->test_run_all_maps_batch(key_bytes, data_bytes,
                                        _schedule_flat.data(),
                                        (int)_schedule->entries.size(),
                                        result_out, 1);
    }

    bool test_bruteforce_checksum(uint32_t data_val, int world)
    {
        uint8_t pipeline_out[128];
        test_bruteforce_data(data_val, pipeline_out);
        return _check_checksum(pipeline_out, world);
    }

private:
    RNG* _rng;
    uint32_t _key;
    std::optional<key_schedule> _schedule;
    std::vector<uint8_t> _schedule_flat;

    static opencl* _s_cl;
    static tm_opencl_seq* _s_gpu;

    static void _ensure_gpu(RNG* rng)
    {
        if (!_s_cl)
        {
            _s_cl  = new opencl(0, 0);
            _s_gpu = new tm_opencl_seq(rng, _s_cl);
        }
    }

    void _build_schedule_flat()
    {
        if (!_schedule) return;
        int n = (int)_schedule->entries.size();
        _schedule_flat.resize(n * 4);
        for (int m = 0; m < n; m++)
        {
            _schedule_flat[m * 4 + 0] = _schedule->entries[m].rng1;
            _schedule_flat[m * 4 + 1] = _schedule->entries[m].rng2;
            _schedule_flat[m * 4 + 2] = (uint8_t)(_schedule->entries[m].nibble_selector >> 8);
            _schedule_flat[m * 4 + 3] = (uint8_t)(_schedule->entries[m].nibble_selector & 0xFF);
        }
    }

    static bool _check_checksum(const uint8_t* pipeline_out, int world)
    {
        const uint8_t* world_xor = (world == CARNIVAL_WORLD)
            ? TM_base::carnival_world_data
            : TM_base::other_world_data;
        const uint8_t* mask = (world == CARNIVAL_WORLD)
            ? TM_base::carnival_world_checksum_mask
            : TM_base::other_world_checksum_mask;

        uint8_t decrypted[128];
        for (int i = 0; i < 128; i++)
            decrypted[i] = pipeline_out[i] ^ world_xor[i];

        uint16_t checksum = 0;
        for (int i = 0; i < 128; i++)
            checksum += decrypted[i] & mask[i];

        int code_length = (world == CARNIVAL_WORLD)
            ? CARNIVAL_WORLD_CODE_LENGTH
            : OTHER_WORLD_CODE_LENGTH;
        uint16_t stored = (uint16_t)((uint16_t)decrypted[127 - (code_length - 1)] << 8)
                        | decrypted[127 - (code_length - 2)];
        return checksum == stored;
    }
};

inline opencl*        tm_opencl_seq_test::_s_cl  = nullptr;
inline tm_opencl_seq* tm_opencl_seq_test::_s_gpu = nullptr;
