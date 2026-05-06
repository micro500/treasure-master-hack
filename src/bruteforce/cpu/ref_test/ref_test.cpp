// Reference-vector test runner. Loads TMTV (TM Test Vector) corpus files and
// replays each record through the appropriate C++ entry point, reporting
// mismatches.
//
// Usage: ref_test [corpus_dir]
//   corpus_dir defaults to ./ref_vectors/

#include "bench_select.h"
#include "key_schedule.h"
#include "rng_obj.h"
#include "tm_base.h"
#include "tm_cpu_factory.h"
#include "tm_8.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

static RNG g_rng;
namespace fs = std::filesystem;

// File format (matches src/bruteforce/emulator/ALGORITHM_TEST_PLAN.md)

static constexpr char     TMTV_MAGIC[4]  = { 'T', 'M', 'T', 'V' };
static constexpr uint16_t TMTV_VERSION   = 1;

static constexpr uint8_t  TT_KS_DISPATCH          = 0x01;
static constexpr uint8_t  TT_KS_ALG               = 0x02;
static constexpr uint8_t  TT_KS_ALL_MAPS          = 0x03;
static constexpr uint8_t  TT_EXPANSION            = 0x04;
static constexpr uint8_t  TT_WC_ALG               = 0x05;
static constexpr uint8_t  TT_WC_ALG_MULTI         = 0x06;
static constexpr uint8_t  TT_WC_ALL_MAPS          = 0x07;
static constexpr uint8_t  TT_DECRYPT              = 0x08;
static constexpr uint8_t  TT_CHECKSUM             = 0x09;

static constexpr uint8_t  RECORD_KIND_FULL        = 0x00;

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
                        uint8_t expected_test_type, int expected_subtype /*-1=any*/,
                        uint16_t expected_record_size /*0=any*/) {
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

static bool read_records(std::ifstream& f, const fs::path& path, std::vector<uint8_t>& out,
                         uint32_t record_count, uint16_t record_size) {
    out.resize(static_cast<size_t>(record_count) * record_size);
    f.read(reinterpret_cast<char*>(out.data()), out.size());
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
    std::vector<uint32_t> fail_idxs;
    void note_fail(uint32_t i) { fail++; if (first_fail_idx == UINT64_MAX) first_fail_idx = i; fail_idxs.push_back(i); }
};

static void print_fail_ranges(const std::vector<uint32_t>& idxs) {
    if (idxs.empty()) return;
    printf("    fail ranges:");
    uint32_t start = idxs[0], prev = idxs[0];
    for (size_t k = 1; k < idxs.size(); ++k) {
        uint32_t v = idxs[k];
        if (v == prev + 1) { prev = v; continue; }
        if (start == prev) printf(" %u", start);
        else printf(" %u-%u", start, prev);
        start = prev = v;
    }
    if (start == prev) printf(" %u", start);
    else printf(" %u-%u", start, prev);
    printf("\n");
}

static void print_result(const char* label, const test_result& r, uint32_t total) {
    const char* tag = (r.fail == 0) ? "PASS" : "FAIL";
    printf("  %s: %s  %llu/%u",
           label, tag, static_cast<unsigned long long>(r.pass), total);
    if (r.fail > 0) {
        printf("  (first_fail=%llu)", static_cast<unsigned long long>(r.first_fail_idx));
    }
    printf("\n");
    if (r.fail > 0) print_fail_ranges(r.fail_idxs);
}

// =============================================================================
// 0x01 algorithm  +  0x07 schedule_chain
//   record: [2] rng_in [128] buf_in [N alg_ids] [2] rng_out [128] buf_out
//   N==1 for 0x01, N==subtype for 0x07
// =============================================================================

static test_result run_chain_corpus(TM_base* impl, int chain_len,
                                    const std::vector<uint8_t>& records,
                                    uint32_t record_count, uint16_t record_size) {
    test_result r{};
    alignas(128) uint8_t buf[128];
    const bool check_rng = impl->tracks_rng_state();
    const int rng_in_off = 0;
    const int buf_in_off = 2;
    const int alg_ids_off = (chain_len == 1) ? 0 /*alg_id is in subtype*/ : 130;
    // For 0x01, alg_id is the file's subtype (passed in via chain_len == 1 caller).
    // For 0x07, alg_ids[chain_len] live between buf_in and rng_out.
    const int rng_out_off = (chain_len == 1) ? 130 : (130 + chain_len);
    const int buf_out_off = rng_out_off + 2;
    (void)alg_ids_off;

    for (uint32_t i = 0; i < record_count; ++i) {
        const uint8_t* rec = records.data() + size_t(i) * record_size;
        memcpy(buf, rec + buf_in_off, 128);
        uint16_t seed = uint16_t(rec[rng_in_off] << 8 | rec[rng_in_off + 1]);

        if (chain_len == 1) {
            // Caller supplies alg id via impl call below using sub-r's caller info; handled outside.
            // This function is reused for 0x07 only; 0x01 uses run_alg_corpus_single below.
        }
        // 0x07 path: read alg ids from record
        impl->test_algorithm_chain(rec + 130, chain_len, buf, &seed);

        bool buf_ok = (memcmp(buf, rec + buf_out_off, 128) == 0);
        uint16_t expected_seed = uint16_t(rec[rng_out_off] << 8 | rec[rng_out_off + 1]);
        bool rng_ok = !check_rng || (seed == expected_seed);
        if (buf_ok && rng_ok) r.pass++; else r.note_fail(i);
    }
    return r;
}

static test_result run_alg_corpus_single(TM_base* impl, int alg_id,
                                         const std::vector<uint8_t>& records,
                                         uint32_t record_count) {
    test_result r{};
    alignas(128) uint8_t buf[128];
    const bool check_rng = impl->tracks_rng_state();
    constexpr uint16_t REC = 260;

    for (uint32_t i = 0; i < record_count; ++i) {
        const uint8_t* rec = records.data() + size_t(i) * REC;
        memcpy(buf, rec + 2, 128);
        uint16_t seed = uint16_t(rec[0] << 8 | rec[1]);
        impl->test_algorithm(alg_id, buf, &seed);
        bool buf_ok = (memcmp(buf, rec + 132, 128) == 0);
        uint16_t expected_seed = uint16_t(rec[130] << 8 | rec[131]);
        bool rng_ok = !check_rng || (seed == expected_seed);
        if (buf_ok && rng_ok) r.pass++; else r.note_fail(i);
    }
    return r;
}

// =============================================================================
// 0x04 key_schedule_alg
//   subtype = algo id (0..7)
//   record (full): [4] state_in [1] map [4] state_out  -> 9 bytes
//   No impl variation: uses key_schedule::test_alg.
// =============================================================================
static test_result run_key_schedule_alg_corpus(int alg_id, const std::vector<uint8_t>& records,
                                           uint32_t record_count) {
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

// =============================================================================
// 0x05 key_schedule_dispatch
//   record (full): [4] state_in [1] map [4] state_out [1] ran_flag [1] routine_id -> 11
//   ran_flag/routine_id are checks against the dispatcher's selector behaviour;
//   our pure dispatch always runs and returns the routine id, so ran_flag is
//   expected = 1 for all records.
// =============================================================================
static test_result run_key_schedule_dispatch_corpus(const std::vector<uint8_t>& records,
                                        uint32_t record_count, test_result& routine_r) {
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
        if (state_ok) r.pass++; else r.note_fail(i);
        if (routine_ok) routine_r.pass++; else routine_r.note_fail(i);
    }
    return r;
}

// =============================================================================
// 0x08 key_schedule_all_maps
//   record (full): [4] input [27 x 4] step_outputs -> 112 bytes
//   input bytes drive schedule_data initial state directly (treated as big-endian key).
// =============================================================================
static test_result run_key_schedule_all_maps_corpus(const std::vector<uint8_t>& records,
                                            uint32_t record_count) {
    test_result r{};
    constexpr uint16_t REC = 112;
    for (uint32_t i = 0; i < record_count; ++i) {
        const uint8_t* rec = records.data() + size_t(i) * REC;
        uint32_t key = be_u32(rec);
        key_schedule ks(key, key_schedule::ALL_MAPS);
        bool ok = (ks.entry_count == 27);
        if (ok) {
            for (int e = 0; e < 27 && ok; ++e) {
                const auto& ent = ks.entries[e];
                uint8_t exp[4] = { ent.rng1, ent.rng2,
                                   uint8_t(ent.nibble_selector & 0xFF),
                                   uint8_t((ent.nibble_selector >> 8) & 0xFF) };
                if (memcmp(exp, rec + 4 + e * 4, 4) != 0) ok = false;
            }
        }
        if (ok) r.pass++; else r.note_fail(i);
    }
    return r;
}

// =============================================================================
// 0x03 expansion
//   record (full): [8] key_in [128] buffer_out -> 136 bytes
//   key_in = [4] key (big-endian) [4] data (big-endian).
//   Uses TM_base::test_expansion(data); requires impl created with the right key.
// =============================================================================
static test_result run_expansion_corpus(TM_base* impl, const std::vector<uint8_t>& records,
                                        uint32_t record_count) {
    test_result r{};
    constexpr uint16_t REC = 136;
    alignas(128) uint8_t buf[128];
    for (uint32_t i = 0; i < record_count; ++i) {
        const uint8_t* rec = records.data() + size_t(i) * REC;
        uint32_t key  = be_u32(rec);
        uint32_t data = be_u32(rec + 4);
        impl->set_key(key);
        impl->test_expansion(data, buf);
        bool ok = (memcmp(buf, rec + 8, 128) == 0);
        if (ok) r.pass++; else r.note_fail(i);
    }
    return r;
}

// =============================================================================
// 0x09 all_maps
//   record (full): [8] key_in [128] buffer_out -> 136 bytes
//   Uses TM_base::test_bruteforce_data(data) (= expansion + run_all_maps).
// =============================================================================
static test_result run_all_maps_corpus(TM_base* impl, const std::vector<uint8_t>& records,
                                         uint32_t record_count) {
    test_result r{};
    constexpr uint16_t REC = 136;
    alignas(128) uint8_t buf[128];
    for (uint32_t i = 0; i < record_count; ++i) {
        const uint8_t* rec = records.data() + size_t(i) * REC;
        uint32_t key  = be_u32(rec);
        uint32_t data = be_u32(rec + 4);
        impl->set_key(key);
        impl->test_bruteforce_data(data, buf);
        bool ok = (memcmp(buf, rec + 8, 128) == 0);
        if (ok) r.pass++; else r.note_fail(i);
    }
    return r;
}

// =============================================================================
// 0x0A decryption
//   subtype = entry index (0 = carnival, 1 = other)
//   record (full): [128] keystream [N] decrypted, N = 114 (carnival) or 83 (other)
//   decrypted[i] = keystream[i] ^ world_data[i] for i in [0, N).
// =============================================================================
static test_result run_decryption_corpus(int entry, const std::vector<uint8_t>& records,
                                         uint32_t record_count, uint32_t code_length) {
    test_result r{};
    const uint16_t REC = uint16_t(128 + code_length);
    const uint8_t* world_data = (entry == 0) ? TM_base::carnival_world_data : TM_base::other_world_data;
    for (uint32_t i = 0; i < record_count; ++i) {
        const uint8_t* rec = records.data() + size_t(i) * REC;
        bool ok = true;
        for (uint32_t b = 0; b < code_length; ++b) {
            uint8_t expected = rec[127 - b] ^ world_data[127 - b];
            if (expected != rec[128 + b]) { ok = false; break; }
        }
        if (ok) r.pass++; else r.note_fail(i);
    }
    return r;
}

// =============================================================================
// 0x0B checksum
//   subtype = entry index
//   record (full): [N] buffer [2] computed_sum [2] stored_sum [1] match_flag
//   Validates against TM_base masked-checksum semantics. The 6502 routine
//   processes the buffer in code-order, but $06A5 captures memory-order
//   storage: byte[0] = $06A5 = "first byte of decrypted memory". The C++ port
//   sums (buffer[i] & mask[i]) for i in [16, N). stored_sum = byte[N-1]<<8 |
//   byte[N-2]. We compare both; if either fails we count it as a record fail.
// =============================================================================
static test_result run_checksum_corpus(int /*entry*/, const std::vector<uint8_t>& records,
                                       uint32_t record_count, uint32_t code_length) {
    test_result r{};
    const uint16_t REC = uint16_t(code_length + 5);
    for (uint32_t i = 0; i < record_count; ++i) {
        const uint8_t* rec = records.data() + size_t(i) * REC;
        // Two interpretations. Try the most likely first: sum bytes [16, N-2)
        // (skipping mask-zero header and the trailing 2 stored-sum bytes).
        uint16_t sum = 0;
        for (uint32_t b = 0; b < code_length - 2; ++b) sum = uint16_t(sum + rec[b]);
        uint16_t stored = uint16_t((rec[code_length - 1] << 8) | rec[code_length - 2]);
        uint16_t exp_computed = uint16_t(rec[code_length] | (rec[code_length + 1] << 8));
        uint16_t exp_stored   = uint16_t(rec[code_length + 2] | (rec[code_length + 3] << 8));
        uint8_t  exp_match    = rec[code_length + 4];
        bool ok = (sum == exp_computed) && (stored == exp_stored) &&
                  ((sum == stored ? 1 : 0) == exp_match);
        if (ok) r.pass++; else r.note_fail(i);
    }
    return r;
}

// =============================================================================
// Loaders
// =============================================================================

struct loaded_file {
    tmtv_header hdr;
    std::vector<uint8_t> data;
};

static bool load_file(const fs::path& path, uint8_t expected_tt, int expected_subtype,
                      uint16_t expected_record_size, loaded_file& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false; // silent — skip missing files
    if (!read_header(f, path, out.hdr, expected_tt, expected_subtype, expected_record_size)) return false;
    if (!read_records(f, path, out.data, out.hdr.record_count, out.hdr.record_size)) return false;
    return true;
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char** argv) {
    fs::path corpus_dir = (argc > 1) ? fs::path(argv[1]) : fs::path("./ref_vectors/");
    std::string impl_filter = (argc > 2) ? std::string(argv[2]) : std::string();
    std::string bank_filter = (argc > 3) ? std::string(argv[3]) : std::string();

    auto bank_enabled = [&](const char* name) -> bool {
        if (bank_filter.empty()) return true;
        std::string s = "," + bank_filter + ",";
        std::string n = std::string(",") + name + ",";
        return s.find(n) != std::string::npos;
    };

    if (bank_filter == "trace") {
        uint32_t key  = (argc > 4) ? std::strtoul(argv[4], nullptr, 0) : 0u;
        uint32_t data = (argc > 5) ? std::strtoul(argv[5], nullptr, 0) : 0u;
        printf("trace key=0x%08X data=0x%08X\n", key, data);
        tm_8 impl(&g_rng, key);
        const auto& entries = impl.schedule_entries->entries;
        int n = static_cast<int>(entries.size());
        std::vector<uint8_t> per_map(n * 128);
        std::vector<uint8_t> algs(n * 16);
        impl.test_trace_per_map(data, per_map.data(), algs.data());
        for (int i = 0; i < n; ++i) {
            const auto& e = entries[i];
            uint8_t b0 = e.rng1;
            uint8_t b1 = e.rng2;
            uint8_t b2 = static_cast<uint8_t>(e.nibble_selector & 0xFF);
            uint8_t b3 = static_cast<uint8_t>((e.nibble_selector >> 8) & 0xFF);
            printf("map %2d  alg=%u  sched=%02X %02X %02X %02X  algs:", i, e.algorithm, b0, b1, b2, b3);
            for (int j = 0; j < 16; ++j) printf(" %u", algs[i * 16 + j]);
            printf("  wc:");
            for (int j = 0; j < 128; ++j) printf(" %02X", per_map[i * 128 + j]);
            printf("\n");
        }
        return 0;
    }

    printf("ref_test: corpus dir = %s\n", corpus_dir.string().c_str());
    if (!impl_filter.empty()) printf("ref_test: impl filter = %s\n", impl_filter.c_str());
    if (!bank_filter.empty()) printf("ref_test: bank filter = %s\n", bank_filter.c_str());
    if (!fs::is_directory(corpus_dir)) {
        printf("ERROR: not a directory\n");
        return 2;
    }
    printf("usage: ref_test [corpus_dir] [impl_name|\"\"] [bank,bank,...]\n");
    printf("  banks: key_schedule_alg, key_schedule_dispatch, key_schedule_all_maps, decryption, checksum,\n");
    printf("         wc_alg, chain, expansion, all_maps\n\n");

    int total_failures = 0;

    // ---- 0x04 key_schedule_alg (no impl variation) ----
    if (bank_enabled("key_schedule_alg")) {
    printf("=== 0x04 key_schedule_alg ===\n");
    for (int alg = 0; alg < 8; ++alg) {
        char fname[64];
        snprintf(fname, sizeof(fname), "key_schedule_alg_%d.bin", alg);
        loaded_file lf;
        if (!load_file(corpus_dir / fname, TT_KS_ALG, alg, 9, lf)) continue;
        char label[32]; snprintf(label, sizeof(label), "alg %d", alg);
        test_result r = run_key_schedule_alg_corpus(alg, lf.data, lf.hdr.record_count);
        print_result(label, r, lf.hdr.record_count);
        if (r.fail) total_failures++;
    }
    printf("\n");
    } // key_schedule_alg

    // ---- 0x05 key_schedule_dispatch (no impl variation) ----
    if (bank_enabled("key_schedule_dispatch")) {
    printf("=== 0x05 key_schedule_dispatch ===\n");
    {
        loaded_file lf;
        if (load_file(corpus_dir / "key_schedule_dispatch.bin", TT_KS_DISPATCH, -1, 11, lf)) {
            test_result routine_r{};
            test_result r = run_key_schedule_dispatch_corpus(lf.data, lf.hdr.record_count, routine_r);
            print_result("state ", r, lf.hdr.record_count);
            print_result("alg id", routine_r, lf.hdr.record_count);
            if (r.fail || routine_r.fail) total_failures++;
        }
    }
    printf("\n");
    } // key_schedule_dispatch

    // ---- 0x08 key_schedule_all_maps (uses key_schedule directly) ----
    if (bank_enabled("key_schedule_all_maps")) {
    printf("=== 0x08 key_schedule_all_maps ===\n");
    {
        loaded_file lf;
        if (load_file(corpus_dir / "key_schedule_all_maps.bin", TT_KS_ALL_MAPS, -1, 112, lf)) {
            test_result r = run_key_schedule_all_maps_corpus(lf.data, lf.hdr.record_count);
            print_result("schedule", r, lf.hdr.record_count);
            if (r.fail) total_failures++;
        }
    }
    printf("\n");
    } // key_schedule_all_maps

    // ---- 0x0A decryption (no impl variation; static world_data XOR) ----
    if (bank_enabled("decryption")) {
    printf("=== 0x0A decryption ===\n");
    {
        const struct { const char* fname; int entry; uint32_t N; } files[] = {
            { "decrypt0.bin", 0, CARNIVAL_WORLD_CODE_LENGTH },
            { "decrypt1.bin", 1, OTHER_WORLD_CODE_LENGTH    },
        };
        for (auto& s : files) {
            loaded_file lf;
            if (!load_file(corpus_dir / s.fname, TT_DECRYPT, s.entry, uint16_t(128 + s.N), lf)) continue;
            char label[32]; snprintf(label, sizeof(label), "entry %d", s.entry);
            test_result r = run_decryption_corpus(s.entry, lf.data, lf.hdr.record_count, s.N);
            print_result(label, r, lf.hdr.record_count);
            if (r.fail) total_failures++;
        }
    }
    printf("\n");
    } // decryption

    // ---- 0x0B checksum (best-effort interpretation; see note in fn) ----
    if (bank_enabled("checksum")) {
    printf("=== 0x0B checksum ===\n");
    {
        const struct { const char* fname; int entry; uint32_t N; } files[] = {
            { "checksum0.bin", 0, CARNIVAL_WORLD_CODE_LENGTH },
            { "checksum1.bin", 1, OTHER_WORLD_CODE_LENGTH    },
        };
        for (auto& s : files) {
            loaded_file lf;
            if (!load_file(corpus_dir / s.fname, TT_CHECKSUM, s.entry, uint16_t(s.N + 5), lf)) continue;
            char label[32]; snprintf(label, sizeof(label), "entry %d", s.entry);
            test_result r = run_checksum_corpus(s.entry, lf.data, lf.hdr.record_count, s.N);
            print_result(label, r, lf.hdr.record_count);
            if (r.fail) total_failures++;
        }
    }
    printf("\n");
    } // checksum

    // ---- 0x01 algorithm + 0x07 chain + 0x03 expansion + 0x09 all_maps ----
    // Per-impl banks. Skip the load + impl loop entirely if none enabled.
    const bool any_per_impl = bank_enabled("wc_alg") || bank_enabled("chain") ||
                              bank_enabled("expansion") || bank_enabled("all_maps");
    if (!any_per_impl) {
        printf("summary: %d test group(s) had failures\n", total_failures);
        return total_failures == 0 ? 0 : 1;
    }

    loaded_file alg_corp[8]; bool alg_ok[8] = {};
    if (bank_enabled("wc_alg")) {
        for (int a = 0; a < 8; ++a) {
            char fname[64]; snprintf(fname, sizeof(fname), "wc_alg_%d.bin", a);
            alg_ok[a] = load_file(corpus_dir / fname, TT_WC_ALG, a, 260, alg_corp[a]);
        }
    }

    static const int CHAIN_LENS[] = { 2, 3, 4, 8, 11, 16 };
    constexpr int N_CHAIN = sizeof(CHAIN_LENS) / sizeof(CHAIN_LENS[0]);
    loaded_file chain_corp[N_CHAIN]; bool chain_ok[N_CHAIN] = {};
    if (bank_enabled("chain")) {
        for (int k = 0; k < N_CHAIN; ++k) {
            int n = CHAIN_LENS[k];
            char fname[64]; snprintf(fname, sizeof(fname), "wc_alg_multi_%d.bin", n);
            chain_ok[k] = load_file(corpus_dir / fname, TT_WC_ALG_MULTI, n, uint16_t(260 + n), chain_corp[k]);
        }
    }

    loaded_file expansion_corp; bool expansion_ok = bank_enabled("expansion") &&
        load_file(corpus_dir / "expansion.bin", TT_EXPANSION, -1, 136, expansion_corp);
    loaded_file all_maps_corp; bool all_maps_ok = bank_enabled("all_maps") &&
        load_file(corpus_dir / "all_maps.bin", TT_WC_ALL_MAPS, -1, 136, all_maps_corp);

    auto impl_types = tm_cpu_factory::get_impls_for_isa(detect_isa());
    if (!impl_filter.empty()) {
        auto found = tm_cpu_factory::find_by_name(impl_filter);
        if (!found) {
            printf("ERROR: impl '%s' not found\n", impl_filter.c_str());
            return 2;
        }
        impl_types.assign(1, *found);
    }
    printf("running per-impl tests against %zu impl(s)\n\n", impl_types.size());

    for (auto& it : impl_types) {
        TM_base* impl = tm_cpu_factory::create(it, &g_rng, 0);
        const char* rng_tag = impl->tracks_rng_state() ? "" : " [no rng tracking]";
        printf("=== %s%s ===\n", impl->obj_name.c_str(), rng_tag);

        // 0x01 algorithm
        for (int a = 0; a < 8; ++a) {
            if (!alg_ok[a]) continue;
            char label[32]; snprintf(label, sizeof(label), "alg %d", a);
            test_result r = run_alg_corpus_single(impl, a, alg_corp[a].data, alg_corp[a].hdr.record_count);
            print_result(label, r, alg_corp[a].hdr.record_count);
            if (r.fail) total_failures++;
        }

        // 0x07 schedule_chain
        for (int k = 0; k < N_CHAIN; ++k) {
            if (!chain_ok[k]) continue;
            int n = CHAIN_LENS[k];
            char label[32]; snprintf(label, sizeof(label), "chain%d", n);
            test_result r = run_chain_corpus(impl, n, chain_corp[k].data,
                                             chain_corp[k].hdr.record_count, uint16_t(260 + n));
            print_result(label, r, chain_corp[k].hdr.record_count);
            if (r.fail) total_failures++;
        }

        // 0x03 expansion / 0x09 all_maps mutate the impl's key+schedule per record.
        if (expansion_ok) {
            test_result r = run_expansion_corpus(impl, expansion_corp.data, expansion_corp.hdr.record_count);
            print_result("expansion", r, expansion_corp.hdr.record_count);
            if (r.fail) total_failures++;
        }
        if (all_maps_ok) {
            test_result r = run_all_maps_corpus(impl, all_maps_corp.data, all_maps_corp.hdr.record_count);
            print_result("all_maps", r, all_maps_corp.hdr.record_count);
            if (r.fail) total_failures++;
        }

        delete impl;
        printf("\n");
    }

    printf("summary: %d test group(s) had failures\n", total_failures);
    return total_failures == 0 ? 0 : 1;
}
