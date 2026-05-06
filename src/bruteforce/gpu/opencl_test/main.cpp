// OpenCL TMTV corpus runner. Replays the per-impl test banks (expansion,
// all_maps, wc_alg, wc_alg_multi) against tm_opencl_seq via its batch test
// APIs. Impl-independent banks (key_schedule_*, decryption, checksum) are
// covered by ref_test and skipped here.
//
// Usage: opencl_test [corpus_dir] [bank,bank,...]
//   corpus_dir defaults to ./ref_vectors/
//   banks: expansion, all_maps, wc_alg, wc_alg_multi

#include "key_schedule.h"
#include "rng_obj.h"
#include "opencl.h"
#include "tm_opencl_seq.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static constexpr char     TMTV_MAGIC[4] = { 'T', 'M', 'T', 'V' };
static constexpr uint16_t TMTV_VERSION  = 1;

static constexpr uint8_t TT_EXPANSION    = 0x04;
static constexpr uint8_t TT_WC_ALG       = 0x05;
static constexpr uint8_t TT_WC_ALG_MULTI = 0x06;
static constexpr uint8_t TT_WC_ALL_MAPS  = 0x07;

static constexpr uint8_t RECORD_KIND_FULL = 0x00;

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

struct loaded_file {
    tmtv_header hdr;
    std::vector<uint8_t> data;
};

static bool load_file(const fs::path& path, uint8_t expected_tt, int expected_subtype,
                      uint16_t expected_record_size, loaded_file& out)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    f.read(reinterpret_cast<char*>(&out.hdr), sizeof(out.hdr));
    if (!f) return false;
    if (memcmp(out.hdr.magic, TMTV_MAGIC, 4) != 0) return false;
    if (out.hdr.version != TMTV_VERSION) return false;
    if (out.hdr.test_type != expected_tt) return false;
    if (expected_subtype >= 0 && out.hdr.subtype != expected_subtype) return false;
    if (expected_record_size != 0 && out.hdr.record_size != expected_record_size) return false;
    if (out.hdr.record_kind != RECORD_KIND_FULL) return false;
    out.data.resize(size_t(out.hdr.record_count) * out.hdr.record_size);
    f.read(reinterpret_cast<char*>(out.data.data()), out.data.size());
    return bool(f);
}

struct test_result {
    uint64_t pass = 0;
    uint64_t fail = 0;
    uint64_t first_fail_idx = UINT64_MAX;
    std::vector<uint32_t> fail_idxs;
    void note_fail(uint32_t i)
    {
        fail++;
        if (first_fail_idx == UINT64_MAX) first_fail_idx = i;
        fail_idxs.push_back(i);
    }
};

static void print_fail_ranges(const std::vector<uint32_t>& idxs)
{
    if (idxs.empty()) return;
    printf("    fail ranges:");
    uint32_t start = idxs[0], prev = idxs[0];
    for (size_t k = 1; k < idxs.size(); ++k) {
        uint32_t v = idxs[k];
        if (v == prev + 1) { prev = v; continue; }
        if (start == prev) printf(" %u", start);
        else               printf(" %u-%u", start, prev);
        start = prev = v;
    }
    if (start == prev) printf(" %u", start);
    else               printf(" %u-%u", start, prev);
    printf("\n");
}

static void print_result(const char* label, const test_result& r, uint32_t total)
{
    const char* tag = (r.fail == 0) ? "PASS" : "FAIL";
    printf("  %s: %s  %llu/%u",
           label, tag, (unsigned long long)r.pass, total);
    if (r.fail > 0) printf("  (first_fail=%llu)", (unsigned long long)r.first_fail_idx);
    printf("\n");
    if (r.fail > 0) print_fail_ranges(r.fail_idxs);
}

// ─── Bank runners ────────────────────────────────────────────────────────────

// 0x04 expansion: [4]key_be [4]data_be [128]buf_out, REC=136
static test_result run_expansion(tm_opencl_seq& tm, const loaded_file& lf)
{
    test_result r{};
    uint8_t out[128];
    constexpr uint16_t REC = 136;
    for (uint32_t i = 0; i < lf.hdr.record_count; ++i) {
        const uint8_t* rec = lf.data.data() + size_t(i) * REC;
        tm.test_expand_batch(rec, rec + 4, out, 1);
        if (memcmp(out, rec + 8, 128) == 0) r.pass++; else r.note_fail(i);
    }
    return r;
}

// 0x07 all_maps: [4]key_be [4]data_be [128]buf_out, REC=136
static test_result run_all_maps(tm_opencl_seq& tm, const loaded_file& lf)
{
    test_result r{};
    uint8_t out[128];
    constexpr uint16_t REC = 136;
    for (uint32_t i = 0; i < lf.hdr.record_count; ++i) {
        const uint8_t* rec = lf.data.data() + size_t(i) * REC;
        uint32_t key = (uint32_t(rec[0]) << 24) | (uint32_t(rec[1]) << 16)
                     | (uint32_t(rec[2]) <<  8) |  uint32_t(rec[3]);
        key_schedule ks(key, key_schedule::ALL_MAPS);
        int sched_count = (int)ks.entries.size();
        std::vector<uint8_t> sched_flat(sched_count * 4);
        for (int m = 0; m < sched_count; ++m) {
            sched_flat[m * 4 + 0] = ks.entries[m].rng1;
            sched_flat[m * 4 + 1] = ks.entries[m].rng2;
            sched_flat[m * 4 + 2] = (ks.entries[m].nibble_selector >> 8) & 0xFF;
            sched_flat[m * 4 + 3] =  ks.entries[m].nibble_selector       & 0xFF;
        }
        tm.test_run_all_maps_batch(rec, rec + 4, sched_flat.data(), sched_count, out, 1);
        if (memcmp(out, rec + 8, 128) == 0) r.pass++; else r.note_fail(i);
    }
    return r;
}

// 0x05 wc_alg: [2]seed_in [128]buf_in [2]seed_out [128]buf_out, REC=260
static test_result run_wc_alg(tm_opencl_seq& tm, const loaded_file& lf, int alg)
{
    test_result r{};
    uint8_t out[128];
    constexpr uint16_t REC = 260;
    uint8_t alg_id = (uint8_t)alg;
    for (uint32_t i = 0; i < lf.hdr.record_count; ++i) {
        const uint8_t* rec = lf.data.data() + size_t(i) * REC;
        uint16_t seed_in = uint16_t((rec[0] << 8) | rec[1]);
        uint16_t seed_out = 0;
        tm.test_alg_batch(&alg_id, &seed_in, rec + 2, out, &seed_out, 1);
        // _map_-style impl: skip seed_out check, validate buffer only.
        if (memcmp(out, rec + 132, 128) == 0) r.pass++; else r.note_fail(i);
    }
    return r;
}

// 0x06 wc_alg_multi: [2]seed_in [128]buf_in [N]alg_ids [2]seed_out [128]buf_out
static test_result run_wc_alg_multi(tm_opencl_seq& tm, const loaded_file& lf, int n)
{
    test_result r{};
    uint8_t out[128];
    const uint16_t REC = uint16_t(260 + n);
    for (uint32_t i = 0; i < lf.hdr.record_count; ++i) {
        const uint8_t* rec = lf.data.data() + size_t(i) * REC;
        uint16_t seed_in = uint16_t((rec[0] << 8) | rec[1]);
        uint16_t seed_out = 0;
        const uint8_t* alg_ids = rec + 130;
        const int buf_out_off = 130 + n + 2;
        tm.test_wc_alg_multi_batch(alg_ids, n, &seed_in, rec + 2, out, &seed_out, 1);
        if (memcmp(out, rec + buf_out_off, 128) == 0) r.pass++; else r.note_fail(i);
    }
    return r;
}

// ─── Driver ───────────────────────────────────────────────────────────────────

int main(int argc, char** argv)
{
    fs::path corpus_dir = (argc > 1) ? fs::path(argv[1]) : fs::path("./ref_vectors/");
    std::string bank_filter = (argc > 2) ? std::string(argv[2]) : std::string();

    auto bank_enabled = [&](const char* name) -> bool {
        if (bank_filter.empty()) return true;
        std::string s = "," + bank_filter + ",";
        std::string n = std::string(",") + name + ",";
        return s.find(n) != std::string::npos;
    };

    printf("opencl corpus tester: corpus dir = %s\n", corpus_dir.string().c_str());
    if (!bank_filter.empty()) printf("bank filter = %s\n", bank_filter.c_str());
    if (!fs::is_directory(corpus_dir)) {
        printf("ERROR: not a directory\n");
        return 2;
    }

    opencl _cl(0, 0);
    printf("Platform: %s\n",  _cl.get_platform_name());
    printf("Device:   %s\n\n", _cl.get_device_name());

    RNG rng;
    tm_opencl_seq tm(&rng, &_cl);

    printf("=== tm_opencl_seq [no rng tracking] ===\n");
    int total_failures = 0;

    if (bank_enabled("expansion")) {
        loaded_file lf;
        if (load_file(corpus_dir / "expansion.bin", TT_EXPANSION, -1, 136, lf)) {
            test_result r = run_expansion(tm, lf);
            print_result("expansion", r, lf.hdr.record_count);
            if (r.fail) total_failures++;
        }
    }

    if (bank_enabled("all_maps")) {
        loaded_file lf;
        if (load_file(corpus_dir / "all_maps.bin", TT_WC_ALL_MAPS, -1, 136, lf)) {
            test_result r = run_all_maps(tm, lf);
            print_result("all_maps", r, lf.hdr.record_count);
            if (r.fail) total_failures++;
        }
    }

    if (bank_enabled("wc_alg")) {
        for (int a = 0; a < 8; ++a) {
            char fname[64]; snprintf(fname, sizeof(fname), "wc_alg_%d.bin", a);
            loaded_file lf;
            if (!load_file(corpus_dir / fname, TT_WC_ALG, a, 260, lf)) continue;
            char label[32]; snprintf(label, sizeof(label), "alg %d", a);
            test_result r = run_wc_alg(tm, lf, a);
            print_result(label, r, lf.hdr.record_count);
            if (r.fail) total_failures++;
        }
    }

    if (bank_enabled("wc_alg_multi")) {
        static const int LENS[] = { 2, 3, 4, 8, 11, 16 };
        for (int n : LENS) {
            char fname[64]; snprintf(fname, sizeof(fname), "wc_alg_multi_%d.bin", n);
            loaded_file lf;
            if (!load_file(corpus_dir / fname, TT_WC_ALG_MULTI, n, uint16_t(260 + n), lf)) continue;
            char label[32]; snprintf(label, sizeof(label), "wc_alg_multi%d", n);
            test_result r = run_wc_alg_multi(tm, lf, n);
            print_result(label, r, lf.hdr.record_count);
            if (r.fail) total_failures++;
        }
    }

    printf("\nsummary: %d test group(s) had failures\n", total_failures);
    return total_failures == 0 ? 0 : 1;
}
