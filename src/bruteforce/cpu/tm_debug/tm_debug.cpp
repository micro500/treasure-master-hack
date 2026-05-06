#include "tm_8.h"
#include "tm_base.h"
#include "key_schedule.h"
#include "rng_obj.h"
#include <cstdio>
#include <cstdint>

// Test a single iv (8 bytes) against both worlds: run full pipeline and report
// whether the masked checksum matches. Also dump the decrypted handler bytes
// in natural code order (reverse_offset).
static void test_iv(const uint8_t iv[8]) {
    uint32_t key  = ((uint32_t)iv[0] << 24) | ((uint32_t)iv[1] << 16) | ((uint32_t)iv[2] << 8) | iv[3];
    uint32_t data = ((uint32_t)iv[4] << 24) | ((uint32_t)iv[5] << 16) | ((uint32_t)iv[6] << 8) | iv[7];

    printf("iv = %02X %02X %02X %02X %02X %02X %02X %02X  (key=0x%08X data=0x%08X)\n",
           iv[0], iv[1], iv[2], iv[3], iv[4], iv[5], iv[6], iv[7], key, data);

    RNG rng;
    tm_8 impl(&rng, key);

    bool carnival_ok = impl.test_bruteforce_checksum(data, CARNIVAL_WORLD);
    bool other_ok    = impl.test_bruteforce_checksum(data, OTHER_WORLD);
    printf("  carnival checksum: %s\n", carnival_ok ? "PASS" : "fail");
    printf("  other    checksum: %s\n", other_ok    ? "PASS" : "fail");

    // Dump decrypted bytes for both worlds. test_bruteforce_data returns the
    // post-permute xor_table (working_code_data byte order). XOR with
    // world_data byte-for-byte to get decrypted handler, then reverse-index
    // for natural 6502 code order.
    uint8_t xor_table[128];
    impl.test_bruteforce_data(data, xor_table);

    auto dump = [&](const uint8_t* world_data, int code_len, const char* label) {
        uint8_t dec[128];
        for (int i = 0; i < 128; i++) dec[i] = xor_table[i] ^ world_data[i];
        printf("  --- decrypted (%s, %d bytes, natural order) ---\n", label, code_len);
        for (int i = 0; i < code_len; i++) {
            printf("%02X%c", dec[127 - i], (i % 16 == 15 || i == code_len - 1) ? '\n' : ' ');
        }
        // Last 2 bytes are checksum (little-endian)
        uint16_t stored = dec[127 - (code_len - 2)] | (dec[127 - (code_len - 1)] << 8);
        uint16_t sum = 0;
        for (int i = 0; i < code_len - 2; i++) sum += dec[127 - i];
        printf("  stored sum = 0x%04X  computed sum (no mask) = 0x%04X\n", stored, sum);
    };

    dump(TM_base::carnival_world_data, CARNIVAL_WORLD_CODE_LENGTH, "carnival");
    dump(TM_base::other_world_data,    OTHER_WORLD_CODE_LENGTH,    "other");
}

int main() {
    const uint8_t iv[8] = { 0x42, 0x10, 0x34, 0xB9, 0x03, 0x28, 0x00, 0x35 };
    test_iv(iv);
    return 0;
}
