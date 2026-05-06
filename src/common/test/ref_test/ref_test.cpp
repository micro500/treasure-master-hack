// Reference-vector replay for the key_schedule layer. Loads the TMTV
// (TM Test Vector) corpus produced by the emulator (src/emulator/test_outputs/)
// and replays each record through key_schedule entry points.
//
// Usage: ref_test [corpus_dir]
//   corpus_dir defaults to ../../../emulator/test_outputs/

#include "key_schedule.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static constexpr char     TMTV_MAGIC[4]  = { 'T', 'M', 'T', 'V' };
static constexpr uint16_t TMTV_VERSION   = 1;

static constexpr uint8_t  TT_KS_DISPATCH = 0x01;
static constexpr uint8_t  TT_KS_ALG      = 0x02;
static constexpr uint8_t  TT_KS_ALL_MAPS = 0x03;
static constexpr uint8_t  RECORD_KIND_FULL = 0x00;

#pragma pack(push, 1)
struct tmtv_header {
    char     magic[4];
    uint16_t version;
    uint8_t  test_type;
    uint8_t  subtype;
    uint16_t record_size;
    uint32_t record_count;
    uint32_t shared_count;
    uint8_t  record_kind;
    uint8_t  reserved[13];
};
static_assert(sizeof(tmtv_header) == 32, "header must be 32 bytes");
#pragma pack(pop)

static bool read_header(std::ifstream& f, const fs::path& path, tmtv_header& hdr,
                        uint8_t expected_test_type, int expected_subtype,
                        uint16_t expected_record_size) {
    f.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));
    if (!f) { printf("  [skip] %s: short read on header\n", path.string().c_str()); return false; }
    if (memcmp(hdr.magic, TMTV_MAGIC, 4) != 0) { printf("  [skip] %s: bad magic\n", path.string().c_str()); return false; }
    if (hdr.version != TMTV_VERSION) { printf("  [skip] %s: version %u\n", path.string().c_str(), hdr.version); return false; }
    if (hdr.test_type != expected_test_type) {
        printf("  [skip] %s: test_type 0x%02X (expected 0x%02X)\n", path.string().c_str(), hdr.test_type, expected_test_type);
        return false;
    }
    if (expected_subtype >= 0 && hdr.subtype != expected_subtype) {
        printf("  [skip] %s: subtype %u (expected %d)\n", path.string().c_str(), hdr.subtype, expected_subtype);
        return false;
    }
    if (expected_record_size != 0 && hdr.record_size != expected_record_size) {
        printf("  [skip] %s: record_size %u (expected %u)\n", path.string().c_str(), hdr.record_size, expected_record_size);
        return false;
    }
    if (hdr.record_kind != RECORD_KIND_FULL) {
        printf("  [skip] %s: record_kind 0x%02X (expected full=0x00)\n", path.string().c_str(), hdr.record_kind);
        return false;
    }
    return true;
}

struct loaded_file {
    tmtv_header hdr;
    std::vector<uint8_t> data;
};

static bool load_file(const fs::path& path, uint8_t expected_tt, int expected_subtype,
                      uint16_t expected_record_size, loaded_file& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    if (!read_header(f, path, out.hdr, expected_tt, expected_subtype, expected_record_size)) return false;
    out.data.resize(size_t(out.hdr.record_count) * out.hdr.record_size);
    f.read(reinterpret_cast<char*>(out.data.data()), out.data.size());
    if (!f) {
        printf("  [skip] %s: short read on records\n", path.string().c_str());
        return false;
    }
    return true;
}

static uint32_t be_u32(const uint8_t* p) {
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | uint32_t(p[3]);
}

struct test_result {
    uint64_t pass = 0;
    uint64_t fail = 0;
    uint64_t first_fail_idx = UINT64_MAX;
    void note_fail(uint32_t i) { fail++; if (first_fail_idx == UINT64_MAX) first_fail_idx = i; }
};

static void print_result(const char* label, const test_result& r, uint32_t total) {
    const char* tag = (r.fail == 0) ? "PASS" : "FAIL";
    printf("  %s: %s  %llu/%u",
           label, tag, static_cast<unsigned long long>(r.pass), total);
    if (r.fail > 0) {
        printf("  (first_fail=%llu)", static_cast<unsigned long long>(r.first_fail_idx));
    }
    printf("\n");
}

// 0x02 key_schedule_alg
//   subtype = algorithm id (0..7)
//   record: [4] state_in [1] map [4] state_out
static test_result run_alg_corpus(int alg_id, const std::vector<uint8_t>& records, uint32_t record_count) {
    test_result r{};
    constexpr uint16_t REC = 9;
    uint8_t out[4];
    for (uint32_t i = 0; i < record_count; ++i) {
        const uint8_t* rec = records.data() + size_t(i) * REC;
        key_schedule::test_alg(rec, uint8_t(alg_id), rec[4], out);
        if (memcmp(out, rec + 5, 4) == 0) r.pass++; else r.note_fail(i);
    }
    return r;
}

// 0x01 key_schedule_dispatch
//   record: [4] state_in [1] map [4] state_out [1] ran_flag [1] routine_id
static test_result run_dispatch_corpus(const std::vector<uint8_t>& records, uint32_t record_count,
                                       test_result& routine_r) {
    test_result r{};
    constexpr uint16_t REC = 11;
    uint8_t out[4];
    for (uint32_t i = 0; i < record_count; ++i) {
        const uint8_t* rec = records.data() + size_t(i) * REC;
        uint8_t map = rec[4];
        uint8_t alg = key_schedule::dispatch_alg(rec, map);
        key_schedule::test_alg(rec, alg, map, out);
        bool state_ok = (memcmp(out, rec + 5, 4) == 0);
        bool routine_ok = (uint8_t(alg + 0x27) == rec[10]);
        if (state_ok)   r.pass++;         else r.note_fail(i);
        if (routine_ok) routine_r.pass++; else routine_r.note_fail(i);
    }
    return r;
}

// 0x03 key_schedule_all_maps
//   record: [4] key_be [27 x 4] entries
static test_result run_all_maps_corpus(const std::vector<uint8_t>& records, uint32_t record_count) {
    test_result r{};
    constexpr uint16_t REC = 112;
    for (uint32_t i = 0; i < record_count; ++i) {
        const uint8_t* rec = records.data() + size_t(i) * REC;
        uint32_t key = be_u32(rec);
        key_schedule ks(key, key_schedule::ALL_MAPS);
        bool ok = (ks.entry_count == 27);
        for (int e = 0; e < 27 && ok; ++e) {
            const auto& ent = ks.entries[e];
            uint8_t exp[4] = { ent.rng1, ent.rng2,
                               uint8_t(ent.nibble_selector & 0xFF),
                               uint8_t((ent.nibble_selector >> 8) & 0xFF) };
            if (memcmp(exp, rec + 4 + e * 4, 4) != 0) ok = false;
        }
        if (ok) r.pass++; else r.note_fail(i);
    }
    return r;
}

int main(int argc, char** argv) {
    fs::path corpus_dir = (argc > 1)
        ? fs::path(argv[1])
        : fs::path("../../../emulator/test_outputs/");

    printf("ref_test: corpus dir = %s\n", corpus_dir.string().c_str());
    if (!fs::is_directory(corpus_dir)) {
        printf("ERROR: not a directory\n");
        return 2;
    }

    int failures = 0;

    printf("=== key_schedule_alg ===\n");
    for (int alg = 0; alg < 8; ++alg) {
        char fname[64];
        snprintf(fname, sizeof(fname), "key_schedule_alg_%d.bin", alg);
        loaded_file lf;
        if (!load_file(corpus_dir / fname, TT_KS_ALG, alg, 9, lf)) continue;
        char label[32]; snprintf(label, sizeof(label), "alg %d", alg);
        test_result r = run_alg_corpus(alg, lf.data, lf.hdr.record_count);
        print_result(label, r, lf.hdr.record_count);
        if (r.fail) failures++;
    }
    printf("\n");

    printf("=== key_schedule_dispatch ===\n");
    {
        loaded_file lf;
        if (load_file(corpus_dir / "key_schedule_dispatch.bin", TT_KS_DISPATCH, -1, 11, lf)) {
            test_result routine_r{};
            test_result r = run_dispatch_corpus(lf.data, lf.hdr.record_count, routine_r);
            print_result("state ", r, lf.hdr.record_count);
            print_result("alg id", routine_r, lf.hdr.record_count);
            if (r.fail || routine_r.fail) failures++;
        }
    }
    printf("\n");

    printf("=== key_schedule_all_maps ===\n");
    {
        loaded_file lf;
        if (load_file(corpus_dir / "key_schedule_all_maps.bin", TT_KS_ALL_MAPS, -1, 112, lf)) {
            test_result r = run_all_maps_corpus(lf.data, lf.hdr.record_count);
            print_result("schedule", r, lf.hdr.record_count);
            if (r.fail) failures++;
        }
    }
    printf("\n");

    printf("summary: %d test group(s) had failures\n", failures);
    return failures == 0 ? 0 : 1;
}
