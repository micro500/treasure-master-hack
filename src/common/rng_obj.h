#ifndef RNG_OBJ_H
#define RNG_OBJ_H

#include <stdint.h>
#include <memory>
#include <vector>
#include "alignment2.h"

// ---------------------------------------------------------------------------
// Aligned allocation primitives
// ---------------------------------------------------------------------------

struct AlignedDeleter {
    void operator()(void* p) const { aligned_free(p); }
};

template<typename T>
using AlignedPtr = std::unique_ptr<T[], AlignedDeleter>;

template<typename T>
AlignedPtr<T> make_aligned(size_t count, size_t alignment = 64) {
    return AlignedPtr<T>(static_cast<T*>(aligned_malloc(static_cast<int>(count * sizeof(T)), static_cast<int>(alignment))));
}

// ---------------------------------------------------------------------------
// RNGTable<T> — ref-counted shared static table
//
// Holds a weak_ptr so it frees itself when no impl holds a ref to it.
// T* ptr stays set while alive for direct internal access (e.g. run_rng()).
// Impls cache their own raw copy; the returned shared_ptr keeps it alive.
// ---------------------------------------------------------------------------

template<typename T>
struct RNGTable {
    T* ptr = nullptr;

    std::shared_ptr<void> lock() { return _weak.lock(); }

    std::shared_ptr<void> commit() {
        auto s = std::shared_ptr<void>(ptr, AlignedDeleter{});
        _weak = s;
        return s;
    }

private:
    std::weak_ptr<void> _weak;
};

// ---------------------------------------------------------------------------
// RNG class
// ---------------------------------------------------------------------------

class RNG
{
public:
    RNG();

    uint8_t run_rng(uint16_t* rng_seed);

    std::shared_ptr<void> generate_rng_table();

    std::shared_ptr<void> generate_seed_forward();
    std::shared_ptr<void> generate_seed_forward_1();
    std::shared_ptr<void> generate_seed_forward_128();

    void _split(uint8_t** rng_values, bool hi);

    void _generate_regular_rng_values_for_seed(uint8_t* out, uint16_t seed, int entries, bool packing_16);
    std::shared_ptr<void> generate_regular_rng_values_8();
    std::shared_ptr<void> generate_regular_rng_values_128_8_shuffled();
    std::shared_ptr<void> generate_regular_rng_values_256_8_shuffled();
    std::shared_ptr<void> generate_regular_rng_values_512_8_shuffled();
    std::shared_ptr<void> generate_regular_rng_values_8_hi();
    std::shared_ptr<void> generate_regular_rng_values_256_8_shuffled_hi();
    std::shared_ptr<void> generate_regular_rng_values_8_lo();
    std::shared_ptr<void> generate_regular_rng_values_256_8_shuffled_lo();
    std::shared_ptr<void> generate_regular_rng_values_16();

    AlignedPtr<uint8_t> generate_regular_rng_values_for_seeds(uint16_t* seeds, int seed_count, bool packing_16);
    AlignedPtr<uint8_t> generate_regular_rng_values_for_seeds_8(uint16_t* seeds, int seed_count);

    std::shared_ptr<void> generate_expansion_values_8();
    std::shared_ptr<void> generate_expansion_values_128_8_shuffled();
    std::shared_ptr<void> generate_expansion_values_256_8_shuffled();
    std::shared_ptr<void> generate_expansion_values_512_8_shuffled();

    void _generate_expansion_values_for_seed(uint8_t* rng_values, uint16_t rng_seed, bool shuffle, int bits, bool packing_16);
    void _generate_expansion_values_for_seed_8(AlignedPtr<uint8_t>& out, uint16_t rng_seed, bool shuffle, int bits);

    void _generate_alg0_values_for_seed(uint8_t* out, uint16_t seed, int entries, bool packing_16);
    std::shared_ptr<void> generate_alg0_values_8();
    std::shared_ptr<void> generate_alg0_values_128_8_shuffled();
    std::shared_ptr<void> generate_alg0_values_256_8_shuffled();
    std::shared_ptr<void> generate_alg0_values_512_8_shuffled();
    std::shared_ptr<void> generate_alg0_values_16();

    AlignedPtr<uint8_t> generate_alg0_values_for_seeds(uint16_t* seeds, int seed_count, bool packing_16);
    AlignedPtr<uint8_t> generate_alg0_values_for_seeds_8(uint16_t* seeds, int seed_count);

    void _generate_alg2_values_for_seed(uint8_t* out, uint16_t seed, int entries, int bits, bool packing_16);
    std::shared_ptr<void> generate_alg2_values_8_8();
    std::shared_ptr<void> generate_alg2_values_32_8();
    std::shared_ptr<void> generate_alg2_values_32_16();
    std::shared_ptr<void> generate_alg2_values_64_8();
    std::shared_ptr<void> generate_alg2_values_64_16();
    std::shared_ptr<void> generate_alg2_values_128_8();
    std::shared_ptr<void> generate_alg2_values_128_16();
    std::shared_ptr<void> generate_alg2_values_256_8();
    std::shared_ptr<void> generate_alg2_values_256_16();
    std::shared_ptr<void> generate_alg2_values_512_8();

    AlignedPtr<uint8_t> generate_alg2_values_for_seeds(uint16_t* seeds, int seed_count, int bits, bool packing_16);
    AlignedPtr<uint8_t> generate_alg2_values_for_seeds_128_8(uint16_t* seeds, int seed_count);

    std::shared_ptr<void> generate_alg4_values_8();
    std::shared_ptr<void> generate_alg4_values_8_hi();
    std::shared_ptr<void> generate_alg4_values_128_8_shuffled();
    std::shared_ptr<void> generate_alg4_values_256_8_shuffled();
    std::shared_ptr<void> generate_alg4_values_512_8_shuffled();
    std::shared_ptr<void> generate_alg4_values_256_8_shuffled_hi();
    std::shared_ptr<void> generate_alg4_values_8_lo();
    std::shared_ptr<void> generate_alg4_values_256_8_shuffled_lo();
    std::shared_ptr<void> generate_alg4_values_16();

    void _generate_alg5_values_for_seed(uint8_t* out, uint16_t seed, int entries, int bits, bool packing_16);
    std::shared_ptr<void> generate_alg5_values_8_8();
    std::shared_ptr<void> generate_alg5_values_32_8();
    std::shared_ptr<void> generate_alg5_values_32_16();
    std::shared_ptr<void> generate_alg5_values_64_8();
    std::shared_ptr<void> generate_alg5_values_64_16();
    std::shared_ptr<void> generate_alg5_values_128_8();
    std::shared_ptr<void> generate_alg5_values_128_16();
    std::shared_ptr<void> generate_alg5_values_256_8();
    std::shared_ptr<void> generate_alg5_values_256_16();
    std::shared_ptr<void> generate_alg5_values_512_8();

    AlignedPtr<uint8_t> generate_alg5_values_for_seeds(uint16_t* seeds, int seed_count, int bits, bool packing_16);
    AlignedPtr<uint8_t> generate_alg5_values_for_seeds_128_8(uint16_t* seeds, int seed_count);

    void _generate_alg6_values_for_seed(uint8_t* out, uint16_t seed, int entries, bool packing_16);
    std::shared_ptr<void> generate_alg6_values_8();
    std::shared_ptr<void> generate_alg6_values_128_8_shuffled();
    std::shared_ptr<void> generate_alg6_values_256_8_shuffled();
    std::shared_ptr<void> generate_alg6_values_512_8_shuffled();
    std::shared_ptr<void> generate_alg6_values_16();

    AlignedPtr<uint8_t> generate_alg6_values_for_seeds(uint16_t* seeds, int seed_count, bool packing_16);
    AlignedPtr<uint8_t> generate_alg6_values_for_seeds_8(uint16_t* seeds, int seed_count);

    std::shared_ptr<void> generate_alg06_values_8();
    std::shared_ptr<void> generate_alg06_values_128_8_shuffled();

    std::shared_ptr<void> generate_rng_seq_tables();

    static RNGTable<uint16_t> rng_table;
    static RNGTable<uint16_t> rng_seq_table;
    static RNGTable<uint32_t> rng_pos_table;
    static uint32_t           rng_seq_table_size;
    static RNGTable<uint16_t> seed_forward;
    static RNGTable<uint16_t> seed_forward_1;
    static RNGTable<uint16_t> seed_forward_128;

    static RNGTable<uint8_t>  regular_rng_values_8;
    static RNGTable<uint8_t>  regular_rng_values_128_8_shuffled;
    static RNGTable<uint8_t>  regular_rng_values_256_8_shuffled;
    static RNGTable<uint8_t>  regular_rng_values_512_8_shuffled;
    static RNGTable<uint8_t>  regular_rng_values_8_hi;
    static RNGTable<uint8_t>  regular_rng_values_256_8_shuffled_hi;
    static RNGTable<uint8_t>  regular_rng_values_8_lo;
    static RNGTable<uint8_t>  regular_rng_values_256_8_shuffled_lo;
    static RNGTable<uint16_t> regular_rng_values_16;

    static RNGTable<uint8_t> expansion_values_8;
    static RNGTable<uint8_t> expansion_values_128_8_shuffled;
    static RNGTable<uint8_t> expansion_values_256_8_shuffled;
    static RNGTable<uint8_t> expansion_values_512_8_shuffled;

    static RNGTable<uint8_t>  alg0_values_8;
    static RNGTable<uint8_t>  alg0_values_128_8_shuffled;
    static RNGTable<uint8_t>  alg0_values_256_8_shuffled;
    static RNGTable<uint8_t>  alg0_values_512_8_shuffled;
    static RNGTable<uint16_t> alg0_values_16;

    static RNGTable<uint8_t>  alg2_values_8_8;
    static RNGTable<uint32_t> alg2_values_32_8;
    static RNGTable<uint32_t> alg2_values_32_16;
    static RNGTable<uint64_t> alg2_values_64_8;
    static RNGTable<uint64_t> alg2_values_64_16;
    static RNGTable<uint8_t>  alg2_values_128_8;
    static RNGTable<uint8_t>  alg2_values_128_16;
    static RNGTable<uint8_t>  alg2_values_256_8;
    static RNGTable<uint8_t>  alg2_values_256_16;
    static RNGTable<uint8_t>  alg2_values_512_8;

    static RNGTable<uint8_t>  alg4_values_8;
    static RNGTable<uint8_t>  alg4_values_8_hi;
    static RNGTable<uint8_t>  alg4_values_128_8_shuffled;
    static RNGTable<uint8_t>  alg4_values_256_8_shuffled;
    static RNGTable<uint8_t>  alg4_values_512_8_shuffled;
    static RNGTable<uint8_t>  alg4_values_256_8_shuffled_hi;
    static RNGTable<uint8_t>  alg4_values_8_lo;
    static RNGTable<uint8_t>  alg4_values_256_8_shuffled_lo;
    static RNGTable<uint16_t> alg4_values_16;

    static RNGTable<uint8_t>  alg5_values_8_8;
    static RNGTable<uint32_t> alg5_values_32_8;
    static RNGTable<uint32_t> alg5_values_32_16;
    static RNGTable<uint64_t> alg5_values_64_8;
    static RNGTable<uint64_t> alg5_values_64_16;
    static RNGTable<uint8_t>  alg5_values_128_8;
    static RNGTable<uint8_t>  alg5_values_128_16;
    static RNGTable<uint8_t>  alg5_values_256_8;
    static RNGTable<uint8_t>  alg5_values_256_16;
    static RNGTable<uint8_t>  alg5_values_512_8;

    static RNGTable<uint8_t>  alg6_values_8;
    static RNGTable<uint8_t>  alg6_values_128_8_shuffled;
    static RNGTable<uint8_t>  alg6_values_256_8_shuffled;
    static RNGTable<uint8_t>  alg6_values_512_8_shuffled;
    static RNGTable<uint16_t> alg6_values_16;

    static RNGTable<uint8_t> alg06_values_8;
    static RNGTable<uint8_t> alg06_values_128_8_shuffled;

    std::shared_ptr<void> _rng_table_ref;

private:
    std::shared_ptr<void> _generate_regular_rng_values(RNGTable<uint8_t>& table, bool shuffle, int bits, bool packing_16);
    std::shared_ptr<void> _generate_regular_rng_values(RNGTable<uint16_t>& table, bool shuffle, int bits, bool packing_16);
    std::shared_ptr<void> _generate_regular_rng_values_8_split(RNGTable<uint8_t>& table, bool shuffle, int bits, bool hi);

    std::shared_ptr<void> _generate_expansion_values(RNGTable<uint8_t>& table, bool shuffle, int bits, bool packing_16);

    std::shared_ptr<void> _generate_alg0_values(RNGTable<uint8_t>& table, bool shuffle, int bits, bool packing_16);
    std::shared_ptr<void> _generate_alg0_values(RNGTable<uint16_t>& table, bool shuffle, int bits, bool packing_16);

    std::shared_ptr<void> _generate_alg2_values(RNGTable<uint8_t>& table, int bits, bool packing_16);
    std::shared_ptr<void> _generate_alg2_values(RNGTable<uint32_t>& table, int bits, bool packing_16);
    std::shared_ptr<void> _generate_alg2_values(RNGTable<uint64_t>& table, int bits, bool packing_16);

    std::shared_ptr<void> _generate_alg4_values(RNGTable<uint8_t>& table, bool shuffle, int bits, bool packing_16);
    std::shared_ptr<void> _generate_alg4_values(RNGTable<uint16_t>& table, bool shuffle, int bits, bool packing_16);
    std::shared_ptr<void> _generate_alg4_values_8_split(RNGTable<uint8_t>& table, bool shuffle, int bits, bool hi);

    std::shared_ptr<void> _generate_alg5_values(RNGTable<uint8_t>& table, int bits, bool packing_16);
    std::shared_ptr<void> _generate_alg5_values(RNGTable<uint32_t>& table, int bits, bool packing_16);
    std::shared_ptr<void> _generate_alg5_values(RNGTable<uint64_t>& table, int bits, bool packing_16);

    std::shared_ptr<void> _generate_alg6_values(RNGTable<uint8_t>& table, bool shuffle, int bits, bool packing_16);
    std::shared_ptr<void> _generate_alg6_values(RNGTable<uint16_t>& table, bool shuffle, int bits, bool packing_16);

    std::shared_ptr<void> _generate_alg06_values(RNGTable<uint8_t>& table, bool shuffle, int bits, bool packing_16);
};

#endif // RNG_OBJ_H
