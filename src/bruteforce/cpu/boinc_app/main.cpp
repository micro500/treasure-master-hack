#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <chrono>

#ifdef BOINCAPP
#ifdef _WIN32
#include "boinc_win.h"
#else
#include "config.h"
#endif
#include "boinc_api.h"
#include "filesys.h"
#endif

extern "C" {
#include "sha256.h"
}

#include "rng_obj.h"
#include "bench_select.h"

// -------------------------------------------------------------------
// Constants
// -------------------------------------------------------------------
static const uint32_t CHECK_INTERVAL      = 1u << 18;
static const int      CHALLENGE_COUNT     = 100;
static const int      RESULT_ENTRY_SIZE   = 5;          // 4B LSB + 1B flags
static const int      CHALLENGE_ENTRY_SIZE = 6;         // 4B LSB + 1B carnival + 1B other
static const int      SHA256_SIZE         = 32;

// -------------------------------------------------------------------
// Result entry
// -------------------------------------------------------------------
struct ResultEntry {
	uint32_t lsb;
	uint8_t  flags;
};

// -------------------------------------------------------------------
// Command-line arguments
// -------------------------------------------------------------------
struct Args {
	uint32_t    key_id        = 0;
	uint32_t    range_start   = 0;
	uint32_t    workunit_size = 1u << 24;  // default: 2^24 (desktop BOINC)
	std::string seed;
	std::string impl_name;  // empty = auto-benchmark; short name without "tm_"
};

static Args parse_args(int argc, char** argv)
{
	Args args;
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--key_id") == 0 && i + 1 < argc)
			args.key_id = (uint32_t)strtoul(argv[++i], nullptr, 10);
		else if (strcmp(argv[i], "--range_start") == 0 && i + 1 < argc)
			args.range_start = (uint32_t)strtoul(argv[++i], nullptr, 10);
		else if (strcmp(argv[i], "--workunit_size") == 0 && i + 1 < argc)
			args.workunit_size = (uint32_t)strtoul(argv[++i], nullptr, 10);
		else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc)
			args.seed = argv[++i];
		else if (strcmp(argv[i], "--impl") == 0 && i + 1 < argc)
			args.impl_name = argv[++i];
	}
	return args;
}

// -------------------------------------------------------------------
// Checkpoint I/O
// Format: [uint32 pos][uint32 count][count * 5 bytes entries]
// -------------------------------------------------------------------
static bool read_checkpoint(const char* path, uint32_t& pos, std::vector<ResultEntry>& results)
{
	FILE* f = fopen(path, "rb");
	if (!f) return false;

	uint32_t saved_pos = 0, count = 0;
	bool ok = (fread(&saved_pos, 4, 1, f) == 1) && (fread(&count, 4, 1, f) == 1);
	if (ok) {
		results.resize(count);
		for (uint32_t i = 0; i < count && ok; i++) {
			ok = (fread(&results[i].lsb,   4, 1, f) == 1)
			  && (fread(&results[i].flags,  1, 1, f) == 1);
		}
	}
	fclose(f);
	if (!ok) {
		results.clear();
		return false;
	}
	pos = saved_pos;
	return true;
}

static void write_checkpoint(const char* path, uint32_t pos, const std::vector<ResultEntry>& results)
{
	FILE* f = fopen(path, "wb");
	if (!f) return;
	uint32_t count = (uint32_t)results.size();
	fwrite(&pos,   4, 1, f);
	fwrite(&count, 4, 1, f);
	for (size_t i = 0; i < results.size(); i++) {
		fwrite(&results[i].lsb,   4, 1, f);
		fwrite(&results[i].flags, 1, 1, f);
	}
	fclose(f);
}

// -------------------------------------------------------------------
// Utility: lowercase hex encode
// -------------------------------------------------------------------
static void hex_encode(const uint8_t* data, int len, char* out)
{
	for (int i = 0; i < len; i++)
		sprintf(&out[i * 2], "%02x", (unsigned)data[i]);
	out[len * 2] = '\0';
}

// -------------------------------------------------------------------
// No-op progress callback (outer loop handles BOINC progress reporting)
// -------------------------------------------------------------------
static void noop_progress(double) {}

// -------------------------------------------------------------------
// Application logic (separate function so all locals are destroyed
// before main() returns and the CRT leak check fires)
// -------------------------------------------------------------------
static int run(const Args& args)
{
	// Resolve file paths
	char output_path[512];
	char checkpoint_path[512];
#ifdef BOINCAPP
	boinc_resolve_filename("out",        output_path,     sizeof(output_path));
	boinc_resolve_filename("checkpoint", checkpoint_path, sizeof(checkpoint_path));
#else
	strncpy(output_path,     "out.bin",        sizeof(output_path));
	strncpy(checkpoint_path, "checkpoint.bin", sizeof(checkpoint_path));
#endif

	// Select and construct the impl
	RNG rng;
	std::unique_ptr<TM_base> tm;
	if (!args.impl_name.empty()) {
		auto type = tm_cpu_factory::find_by_name(args.impl_name);
		if (!type) {
			fprintf(stderr, "[TM_IMPL] ERROR: unknown impl '%s'\n", args.impl_name.c_str());
			return 1;
		}
		tm.reset(tm_cpu_factory::create(*type, &rng, args.key_id));
		fprintf(stderr, "[TM_IMPL] %s forced\n", tm->obj_name.c_str());
	} else {
		ISA isa = detect_isa();
		auto results = benchmark_boinc_impls(&rng, args.key_id, isa);
		if (results.empty()) {
			fprintf(stderr, "[TM_IMPL] ERROR: no valid impl found\n");
			return 1;
		}
		for (auto& r : results)
			fprintf(stderr, "[TM_BENCH] %-40s %.1f ns/input\n", r.name.c_str(), r.median_ns);
		tm.reset(tm_cpu_factory::create(results[0].impl_type, &rng, args.key_id));
		fprintf(stderr, "[TM_IMPL] %s bench\n", tm->obj_name.c_str());
	}

	// Restore from checkpoint if present
	uint32_t start_pos = 0;
	std::vector<ResultEntry> results;
	read_checkpoint(checkpoint_path, start_pos, results);

	// Per-key result buffer: worst case all CHECK_INTERVAL inputs produce a result
	const uint32_t key_buf_size = CHECK_INTERVAL * RESULT_ENTRY_SIZE;
	uint8_t* key_buf = new uint8_t[key_buf_size];

	// -------------------------------------------------------------------
	// Main processing loop
	// -------------------------------------------------------------------
#ifdef TIMING
	auto t_start = std::chrono::steady_clock::now();
#endif
	for (uint32_t pos = start_pos; pos < args.workunit_size; pos += CHECK_INTERVAL) {
		uint32_t key_size = CHECK_INTERVAL;
		if (pos + key_size > args.workunit_size)
			key_size = args.workunit_size - pos;

		uint32_t result_bytes = 0;
		tm->run_bruteforce_boinc(
			args.range_start + pos,
			key_size,
			noop_progress,
			key_buf,
			key_buf_size,
			&result_bytes
		);

		uint32_t entry_count = result_bytes / RESULT_ENTRY_SIZE;
		for (uint32_t i = 0; i < entry_count; i++) {
			ResultEntry e;
			memcpy(&e.lsb,   &key_buf[i * RESULT_ENTRY_SIZE], 4);
			e.flags = key_buf[i * RESULT_ENTRY_SIZE + 4];
			results.push_back(e);
		}

		uint32_t next_pos = pos + key_size;
		double fraction = (double)next_pos / args.workunit_size;

#ifdef TIMING
		{
			auto now = std::chrono::steady_clock::now();
			double elapsed = std::chrono::duration<double>(now - t_start).count();
			uint32_t processed = next_pos - start_pos;
			fprintf(stderr, "[timing] %u / %u inputs | %.2fs | %.0f inputs/s\n",
				next_pos, args.workunit_size, elapsed, processed / elapsed);
		}
#endif

#ifdef BOINCAPP
		boinc_fraction_done(fraction);
		if (boinc_time_to_checkpoint()) {
			write_checkpoint(checkpoint_path, next_pos, results);
			boinc_checkpoint_completed();
		}
#endif
	}

	delete[] key_buf;

	// -------------------------------------------------------------------
	// Build entry bytes and compute SHA256
	// -------------------------------------------------------------------
	size_t entry_bytes_size = results.size() * RESULT_ENTRY_SIZE;
	uint8_t* entry_bytes = new uint8_t[entry_bytes_size > 0 ? entry_bytes_size : 1];

	for (size_t i = 0; i < results.size(); i++) {
		memcpy(&entry_bytes[i * RESULT_ENTRY_SIZE], &results[i].lsb, 4);
		entry_bytes[i * RESULT_ENTRY_SIZE + 4] = results[i].flags;
	}

	uint8_t hash[SHA256_SIZE];
	{
		SHA256_CTX ctx;
		sha256_init(&ctx);
		sha256_update(&ctx, entry_bytes, entry_bytes_size);
		sha256_final(&ctx, hash);
	}

	// result_hash_hex = lowercase hex string of hash
	char result_hash_hex[65];
	hex_encode(hash, 32, result_hash_hex);

	// -------------------------------------------------------------------
	// Derive 100 challenge responses
	// Challenge derivation: x = SHA256(result_hash_hex + seed)
	// -------------------------------------------------------------------
	struct Challenge {
		uint32_t lsb;
		uint8_t  carnival_flags;
		uint8_t  other_flags;
	};
	Challenge challenges[CHALLENGE_COUNT];

	std::string commit_input = std::string(result_hash_hex) + args.seed;
	uint8_t x[SHA256_SIZE];
	{
		SHA256_CTX ctx;
		sha256_init(&ctx);
		sha256_update(&ctx, (const uint8_t*)commit_input.data(), commit_input.size());
		sha256_final(&ctx, x);
	}

	for (int i = 0; i < CHALLENGE_COUNT; i++) {
		// First 4 bytes of x as big-endian uint32
		uint32_t x_be = ((uint32_t)x[0] << 24) | ((uint32_t)x[1] << 16)
		               | ((uint32_t)x[2] << 8)  |  (uint32_t)x[3];
		uint32_t challenge_offset = x_be % args.workunit_size;
		uint32_t challenge_lsb    = args.range_start + challenge_offset;

		challenges[i].lsb = challenge_lsb;
		tm->compute_challenge_flags(
			challenge_lsb,
			challenges[i].carnival_flags,
			challenges[i].other_flags
		);

		// Advance chain: x = SHA256(hex(x))
		char x_hex[65];
		hex_encode(x, 32, x_hex);
		{
			SHA256_CTX ctx;
			sha256_init(&ctx);
			sha256_update(&ctx, (const uint8_t*)x_hex, 64);
			sha256_final(&ctx, x);
		}
	}

	// -------------------------------------------------------------------
	// Write output file
	// Layout: [uint32 count][N*5 entries][32 SHA256][100*6 challenges]
	// -------------------------------------------------------------------
	FILE* f = fopen(output_path, "wb");
	if (f) {
		uint32_t count = (uint32_t)results.size();
		fwrite(&count,      4, 1, f);
		fwrite(entry_bytes, 1, entry_bytes_size, f);
		fwrite(hash,        1, SHA256_SIZE, f);
		for (int i = 0; i < CHALLENGE_COUNT; i++) {
			fwrite(&challenges[i].lsb,           4, 1, f);
			fwrite(&challenges[i].carnival_flags, 1, 1, f);
			fwrite(&challenges[i].other_flags,    1, 1, f);
		}
		fclose(f);
	}

	delete[] entry_bytes;

	return 0;
}

// -------------------------------------------------------------------
// main
// -------------------------------------------------------------------
int main(int argc, char** argv)
{
#ifdef BOINCAPP
	boinc_init();
#endif

	int ret = run(parse_args(argc, argv));

#ifdef BOINCAPP
	boinc_finish(ret);
	// boinc_finish does not return
#endif

	return ret;
}
