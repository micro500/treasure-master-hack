#include <gtest/gtest.h>
#include "test_vectors.h"
#include "rng_obj.h"
#include "key_schedule.h"
#include "tm_8.h"
#include "tm_32_8.h"
#include "tm_avx_r128_8.h"
#include "tm_avx_r128s_8.h"
#include "tm_avx_r128s_map_8.h"
#include "tm_avx_r128_map_8.h"
#include "tm_avx_r256_map_8.h"
//#include "tm_opencl_seq_test.h"

// Shared RNG — all implementations use the same tables; each class
// guards its own initialization with a static bool.
static RNG g_rng;

// =========================================================
// Test fixture
// =========================================================

using Implementations = ::testing::Types<tm_8/*, tm_32_8*/, tm_avx_r128_8, tm_avx_r128s_8, tm_avx_r128s_map_8, tm_avx_r128_map_8, tm_avx_r256_map_8/*, *tm_opencl_seq_test*/>;

struct ImplementationNames {
    template <typename T>
    static std::string GetName(int i) {
        if (std::is_same<T, tm_8>()) return "tm_8";
        if (std::is_same<T, tm_32_8>()) return "tm_32_8";
        if (std::is_same<T, tm_avx_r128_8>()) return "tm_avx_r128_8";
        if (std::is_same<T, tm_avx_r128s_8>()) return "tm_avx_r128s_8";
        if (std::is_same<T, tm_avx_r128s_map_8>()) return "tm_avx_r128s_map_8";
        if (std::is_same<T, tm_avx_r128_map_8>()) return "tm_avx_r128_map_8";
        if (std::is_same<T, tm_avx_r256_map_8>()) return "tm_avx_r256_map_8";
        if (std::is_same<T, tm_opencl_seq_test>()) return "tm_opencl_seq_test";

        return std::to_string(i);
    }
};

template<typename T>
class TmTest : public ::testing::Test {
protected:
    std::unique_ptr<T> impl;

    void init() {
        impl = std::make_unique<T>(&g_rng);
    }

    void init(uint32_t key) {
        impl = std::make_unique<T>(&g_rng, key);
    }
};
TYPED_TEST_SUITE(TmTest, Implementations, ImplementationNames);


// =========================================================
// Helper
// =========================================================
template<typename T>
static void run_alg_test(T& impl, int alg_id,
    const uint8* input, uint16 seed_in,
    const uint8* expected, uint16 expected_seed_out)
{
    uint8 output[128];
    for (int i = 0; i < 128; i++)
        output[i] = input[i];
    uint16 seed = seed_in;
    impl.test_algorithm(alg_id, output, &seed);
    for (int i = 0; i < 128; i++)
        EXPECT_EQ(output[i], expected[i]) << "byte " << i;
    //EXPECT_EQ(seed, expected_seed_out) << "rng seed after alg";
}


// =========================================================
// Algorithm tests
// =========================================================
TYPED_TEST(TmTest, Alg0_V0) {
    this->init();
    run_alg_test(*this->impl, 0, alg0_v0_in, alg0_v0_seed_in, alg0_v0_out, alg0_v0_seed_out);
}
TYPED_TEST(TmTest, Alg0_V1) {
    this->init();
    run_alg_test(*this->impl, 0, alg0_v1_in, alg0_v1_seed_in, alg0_v1_out, alg0_v1_seed_out);
}

TYPED_TEST(TmTest, Alg1_V0) {
    this->init();
    run_alg_test(*this->impl, 1, alg1_v0_in, alg1_v0_seed_in, alg1_v0_out, alg1_v0_seed_out);
}
TYPED_TEST(TmTest, Alg1_V1) {
    this->init();
    run_alg_test(*this->impl, 1, alg1_v1_in, alg1_v1_seed_in, alg1_v1_out, alg1_v1_seed_out);
}

TYPED_TEST(TmTest, Alg2_V0) {
    this->init();
    run_alg_test(*this->impl, 2, alg2_v0_in, alg2_v0_seed_in, alg2_v0_out, alg2_v0_seed_out);
}
TYPED_TEST(TmTest, Alg2_V1) {
    this->init();
    run_alg_test(*this->impl, 2, alg2_v1_in, alg2_v1_seed_in, alg2_v1_out, alg2_v1_seed_out);
}

TYPED_TEST(TmTest, Alg3_V0) {
    this->init();
    run_alg_test(*this->impl, 3, alg3_v0_in, alg3_v0_seed_in, alg3_v0_out, alg3_v0_seed_out);
}
TYPED_TEST(TmTest, Alg3_V1) {
    this->init();
    run_alg_test(*this->impl, 3, alg3_v1_in, alg3_v1_seed_in, alg3_v1_out, alg3_v1_seed_out);
}

TYPED_TEST(TmTest, Alg4_V0) {
    this->init();
    run_alg_test(*this->impl, 4, alg4_v0_in, alg4_v0_seed_in, alg4_v0_out, alg4_v0_seed_out);
}
TYPED_TEST(TmTest, Alg4_V1) {
    this->init();
    run_alg_test(*this->impl, 4, alg4_v1_in, alg4_v1_seed_in, alg4_v1_out, alg4_v1_seed_out);
}

TYPED_TEST(TmTest, Alg5_V0) {
    this->init();
    run_alg_test(*this->impl, 5, alg5_v0_in, alg5_v0_seed_in, alg5_v0_out, alg5_v0_seed_out);
}
TYPED_TEST(TmTest, Alg5_V1) {
    this->init();
    run_alg_test(*this->impl, 5, alg5_v1_in, alg5_v1_seed_in, alg5_v1_out, alg5_v1_seed_out);
}

TYPED_TEST(TmTest, Alg6_V0) {
    this->init();
    run_alg_test(*this->impl, 6, alg6_v0_in, alg6_v0_seed_in, alg6_v0_out, alg6_v0_seed_out);
}
TYPED_TEST(TmTest, Alg6_V1) {
    this->init();
    run_alg_test(*this->impl, 6, alg6_v1_in, alg6_v1_seed_in, alg6_v1_out, alg6_v1_seed_out);
}

TYPED_TEST(TmTest, Alg7_V0) {
    this->init();
    run_alg_test(*this->impl, 7, alg7_v0_in, alg7_v0_seed_in, alg7_v0_out, alg7_v0_seed_out);
}
TYPED_TEST(TmTest, Alg7_V1) {
    this->init();
    run_alg_test(*this->impl, 7, alg7_v1_in, alg7_v1_seed_in, alg7_v1_out, alg7_v1_seed_out);
}


// =========================================================
// Expansion tests
// =========================================================
TYPED_TEST(TmTest, Expansion_V0) {
    this->init(expansion_v0_key);
    uint8 out[128];
    this->impl->test_expansion(expansion_v0_data, out);
    for (int i = 0; i < 128; i++)
        EXPECT_EQ(out[i], expansion_v0_out[i]) << "byte " << i;
}

TYPED_TEST(TmTest, Expansion_V1) {
    this->init(expansion_v1_key);
    uint8 out[128];
    this->impl->test_expansion(expansion_v1_data, out);
    for (int i = 0; i < 128; i++)
        EXPECT_EQ(out[i], expansion_v1_out[i]) << "byte " << i;
}


// =========================================================
// Pipeline tests (expand + all maps)
// =========================================================
TYPED_TEST(TmTest, Pipeline_V0) {
    this->init(pipeline_v0_key);
    uint8 out[128];
    this->impl->test_bruteforce_data(pipeline_v0_data, out);
    for (int i = 0; i < 128; i++)
        EXPECT_EQ(out[i], pipeline_v0_out[i]) << "byte " << i;
}

TYPED_TEST(TmTest, Pipeline_V1) {
    this->init(pipeline_v1_key);
    uint8 out[128];
    this->impl->test_bruteforce_data(pipeline_v1_data, out);
    for (int i = 0; i < 128; i++)
        EXPECT_EQ(out[i], pipeline_v1_out[i]) << "byte " << i;
}


// =========================================================
// Checksum validation tests
// =========================================================
TYPED_TEST(TmTest, Validate_CarnivalValid_V0) {
    this->init(validate_carnival_key);
    EXPECT_TRUE(this->impl->test_bruteforce_checksum(validate_carnival_data, CARNIVAL_WORLD));
}

TYPED_TEST(TmTest, Validate_OtherValid_V0) {
    this->init(validate_other_key);
    EXPECT_TRUE(this->impl->test_bruteforce_checksum(validate_other_data, OTHER_WORLD));
}

TYPED_TEST(TmTest, Validate_Invalid_V0) {
    this->init(validate_invalid_key);
    EXPECT_FALSE(this->impl->test_bruteforce_checksum(validate_invalid_data, CARNIVAL_WORLD));
    EXPECT_FALSE(this->impl->test_bruteforce_checksum(validate_invalid_data, OTHER_WORLD));
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
