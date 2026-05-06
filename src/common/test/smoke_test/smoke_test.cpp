// Smoke test: a handful of hardcoded golden vectors pulled from the emulator
// reference corpus (src/emulator/test_outputs/key_schedule_*.bin). Quick
// sanity check that key_schedule wires up correctly without needing the full
// TMTV corpus replay.

#include <gtest/gtest.h>

#include "key_schedule.h"

#include <cstdint>
#include <cstring>

static void run_alg_case(uint8_t alg, const uint8_t in[4], uint8_t map, const uint8_t expected[4]) {
    uint8_t out[4];
    key_schedule::test_alg(in, alg, map, out);
    EXPECT_EQ(0, std::memcmp(out, expected, 4))
        << "alg=" << int(alg) << " map=0x" << std::hex << int(map);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(KeyScheduleAlg, Alg0_Hardcoded) {
    const uint8_t zero[4] = { 0, 0, 0, 0 };
    {
        const uint8_t exp[4] = { 0xff, 0xff, 0xff, 0xff };
        run_alg_case(0, zero, 0x00, exp);
    }
    {
        const uint8_t exp[4] = { 0x01, 0x01, 0x01, 0x01 };
        run_alg_case(0, zero, 0x01, exp);
    }
    {
        const uint8_t exp[4] = { 0x02, 0x02, 0x02, 0x02 };
        run_alg_case(0, zero, 0x02, exp);
    }
}

TEST(KeyScheduleAlg, Alg1_Hardcoded) {
    const uint8_t zero[4] = { 0, 0, 0, 0 };
    {
        const uint8_t exp[4] = { 0x01, 0x01, 0x01, 0x01 };
        run_alg_case(1, zero, 0x00, exp);
    }
    {
        const uint8_t exp[4] = { 0x02, 0x02, 0x02, 0x02 };
        run_alg_case(1, zero, 0x01, exp);
    }
}

TEST(KeyScheduleAlg, Alg2_Hardcoded) {
    const uint8_t zero[4] = { 0, 0, 0, 0 };
    {
        const uint8_t exp[4] = { 0x00, 0x00, 0x00, 0x00 };
        run_alg_case(2, zero, 0x00, exp);
    }
    {
        const uint8_t exp[4] = { 0x00, 0x00, 0x00, 0x01 };
        run_alg_case(2, zero, 0x01, exp);
    }
    {
        const uint8_t exp[4] = { 0x00, 0x00, 0x00, 0x02 };
        run_alg_case(2, zero, 0x02, exp);
    }
}

TEST(KeyScheduleAlg, Alg3_Hardcoded) {
    const uint8_t zero[4] = { 0, 0, 0, 0 };
    {
        const uint8_t exp[4] = { 0x01, 0x01, 0x01, 0x01 };
        run_alg_case(3, zero, 0x00, exp);
    }
    {
        const uint8_t exp[4] = { 0x03, 0x03, 0x03, 0x03 };
        run_alg_case(3, zero, 0x01, exp);
    }
    {
        const uint8_t exp[4] = { 0x05, 0x05, 0x05, 0x05 };
        run_alg_case(3, zero, 0x02, exp);
    }
}

TEST(KeyScheduleAlg, Alg4_Hardcoded) {
    const uint8_t zero[4] = { 0, 0, 0, 0 };
    {
        const uint8_t exp[4] = { 0xff, 0xff, 0xff, 0xff };
        run_alg_case(4, zero, 0x00, exp);
    }
    {
        const uint8_t exp[4] = { 0x02, 0x02, 0x02, 0x02 };
        run_alg_case(4, zero, 0x01, exp);
    }
    {
        const uint8_t exp[4] = { 0x04, 0x04, 0x04, 0x04 };
        run_alg_case(4, zero, 0x02, exp);
    }
}

TEST(KeyScheduleAlg, Alg5_Hardcoded) {
    const uint8_t zero[4] = { 0, 0, 0, 0 };
    {
        const uint8_t exp[4] = { 0xff, 0x00, 0x00, 0x00 };
        run_alg_case(5, zero, 0x00, exp);
    }
    {
        const uint8_t exp[4] = { 0xfc, 0x00, 0x00, 0x00 };
        run_alg_case(5, zero, 0x01, exp);
    }
    {
        const uint8_t exp[4] = { 0xf9, 0x00, 0x00, 0x00 };
        run_alg_case(5, zero, 0x02, exp);
    }
}

TEST(KeyScheduleAlg, Alg6_Hardcoded) {
    const uint8_t zero[4] = { 0, 0, 0, 0 };
    {
        const uint8_t exp[4] = { 0x00, 0x01, 0x01, 0x01 };
        run_alg_case(6, zero, 0x00, exp);
    }
    {
        const uint8_t exp[4] = { 0xfe, 0x02, 0x02, 0x02 };
        run_alg_case(6, zero, 0x01, exp);
    }
    {
        const uint8_t exp[4] = { 0xfc, 0x03, 0x03, 0x03 };
        run_alg_case(6, zero, 0x02, exp);
    }
}

TEST(KeyScheduleAlg, Alg7_Hardcoded) {
    const uint8_t zero[4] = { 0, 0, 0, 0 };
    {
        const uint8_t exp[4] = { 0xfe, 0xff, 0xff, 0xff };
        run_alg_case(7, zero, 0x00, exp);
    }
    {
        const uint8_t exp[4] = { 0xf9, 0xfd, 0xfd, 0xfd };
        run_alg_case(7, zero, 0x01, exp);
    }
    {
        const uint8_t exp[4] = { 0xf4, 0xfb, 0xfb, 0xfb };
        run_alg_case(7, zero, 0x02, exp);
    }
}

TEST(KeyScheduleDispatch, Hardcoded) {
    // From key_schedule_dispatch.bin: all of these map state=00,00,00,00 with
    // the given map value; routine_id 0x27 means alg 0.
    const uint8_t zero[4] = { 0, 0, 0, 0 };
    EXPECT_EQ(0, key_schedule::dispatch_alg(zero, 0x00));
    EXPECT_EQ(0, key_schedule::dispatch_alg(zero, 0x01));
    EXPECT_EQ(0, key_schedule::dispatch_alg(zero, 0x02));
}

TEST(KeyScheduleAllMaps, Key2CA5B42D) {
    // From key_schedule_all_maps.bin record 0: key=0x2CA5B42D big-endian.
    // Records pack each entry as [rng1][rng2][ns_lo][ns_hi].
    key_schedule ks(0x2CA5B42Du, key_schedule::ALL_MAPS);
    ASSERT_EQ(27, ks.entry_count);

    struct expected_entry { uint8_t rng1, rng2; uint16_t ns; };
    const expected_entry want[] = {
        { 0xB4, 0x86, 0x2DD2 },
        { 0xAD, 0x2D, 0xD286 },
        { 0x3E, 0x6B, 0xB8E5 },
        { 0xBD, 0x8B, 0x67D3 },
        { 0x06, 0x52, 0x5FEB },
        { 0xC4, 0x64, 0x2779 },
        { 0xE6, 0x21, 0x44BD },
        { 0x71, 0x8B, 0xAC69 },
        { 0x48, 0xBE, 0x7B38 },
    };
    for (size_t i = 0; i < sizeof(want) / sizeof(want[0]); ++i) {
        const auto& ent = ks.entries[i];
        EXPECT_EQ(want[i].rng1, ent.rng1) << "entry " << i;
        EXPECT_EQ(want[i].rng2, ent.rng2) << "entry " << i;
        EXPECT_EQ(want[i].ns,   ent.nibble_selector) << "entry " << i;
    }
}
