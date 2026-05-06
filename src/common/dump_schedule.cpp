// Small tool: print the 4-byte schedule_data after each entry for a given key.
// Usage: dump_schedule [key_hex]   (default key: 0x2CA5B42D, the known carnival key)
//        dump_schedule [key_hex] --skip-car   (use SKIP_CAR map list)
//
// Build (from a VS x64 Developer prompt, in src/common/):
//   cl /nologo /EHsc /std:c++17 dump_schedule.cpp key_schedule.cpp /Fe:dump_schedule.exe

#include "key_schedule.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

int main(int argc, char** argv)
{
    uint32_t key = 0x2CA5B42Du;
    key_schedule::map_list_type list = key_schedule::ALL_MAPS;

    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--skip-car") == 0)
        {
            list = key_schedule::SKIP_CAR;
        }
        else
        {
            key = std::strtoul(argv[i], nullptr, 0);
        }
    }

    std::printf("key = 0x%08X  list = %s\n", key, list == key_schedule::ALL_MAPS ? "ALL_MAPS" : "SKIP_CAR");
    std::printf("idx  alg   schedule_data (after entry)\n");

    key_schedule ks(key, list);
    for (int i = 0; i < ks.entry_count; ++i)
    {
        const auto& e = ks.entries[i];
        uint8_t b0 = e.rng1;
        uint8_t b1 = e.rng2;
        uint8_t b2 = static_cast<uint8_t>(e.nibble_selector & 0xFF);
        uint8_t b3 = static_cast<uint8_t>((e.nibble_selector >> 8) & 0xFF);
        std::printf("%3d   %u    %02X %02X %02X %02X\n", i, e.algorithm, b0, b1, b2, b3);
    }
    return 0;
}
