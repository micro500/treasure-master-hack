// Convergence analysis experiments.
//
// sweep_keys: for each of num_keys random keys, runs num_inputs data values
//   through the first map only, reports per-key prune rate and average.
//
// full_map_detail: for a specific key, runs num_inputs data values through all
//   26 maps, hashing after every individual algorithm call (416 steps total),
//   and prints a per-step breakdown table.

#include <stdio.h>
#include <stdint.h>
#include <random>
#include <unordered_set>
#include <vector>
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")

#include "data_sizes.h"
#include "rng_obj.h"
#include "tm_base.h"
#include "tm_32_8.h"
#include "key_schedule.h"

static double process_mem_mb()
{
    PROCESS_MEMORY_COUNTERS pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
    return pmc.WorkingSetSize / (1024.0 * 1024.0);
}

static uint64_t fnv1a_64(const uint8* data, size_t len)
{
    uint64_t hash = 14695981039346656037ULL;
    for (size_t i = 0; i < len; i++)
    {
        hash ^= (uint64_t)data[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

// Runs num_inputs sequential data values (starting from 0) through the first
// map only for a single key. Returns the cumulative prune percentage.
static double run_first_map(tm_32_8& processor, uint32 key, int num_inputs)
{
    const int algs_per_map = 16;

    key_schedule schedule_data(key, key_schedule::ALL_MAPS);
    const key_schedule::key_schedule_entry& entry = schedule_data.entries[0];

    std::unordered_set<uint64_t> seen[algs_per_map];
    uint8 state[128];
    uint64_t collisions = 0;

    for (int n = 0; n < num_inputs; n++)
    {
        processor.expand(key, (uint32)n);

        uint16 rng_seed        = ((uint16)entry.rng1 << 8) | entry.rng2;
        uint16 nibble_selector = entry.nibble_selector;

        processor.fetch_data(state);

        bool converged = false;
        for (int i = 0; i < algs_per_map && !converged; i++)
        {
            unsigned char nibble       = (nibble_selector >> 15) & 0x01;
            nibble_selector            = nibble_selector << 1;
            unsigned char current_byte = state[i];
            if (nibble == 1)
                current_byte = current_byte >> 4;
            unsigned char alg_id = (current_byte >> 1) & 0x07;

            processor.run_alg(alg_id, &rng_seed, 1);
            processor.fetch_data(state);

            uint64_t h = fnv1a_64(state, 128);
            if (seen[i].count(h))
            {
                collisions++;
                converged = true;
            }
            else
            {
                seen[i].insert(h);
            }
        }
    }

    return 100.0 * collisions / num_inputs;
}

// Sweeps num_keys random keys, running each through the first map with
// num_inputs sequential data values. Prints per-key prune rate and average.
static void sweep_keys(tm_32_8& processor, int num_keys, int num_inputs)
{
    std::mt19937 key_gen(std::random_device{}());
    double total_prune_pct = 0.0;

    printf("=== sweep_keys: %d keys, %d inputs each, first map only ===\n\n",
           num_keys, num_inputs);

    for (int k = 0; k < num_keys; k++)
    {
        if (k % 50 == 0)
            printf("Progress: %d / %d (%.1f%%)\n", k, num_keys, 100.0 * k / num_keys);

        uint32 key = key_gen();
        double prune_pct = run_first_map(processor, key, num_inputs);
        total_prune_pct += prune_pct;
        printf("Key: 0x%08X  Prune: %6.3f%%\n", key, prune_pct);
    }

    printf("\nAverage prune%% over %d keys (%d inputs each): %.4f%%\n",
           num_keys, num_inputs, total_prune_pct / num_keys);
}

// Runs num_inputs sequential data values through all 26 maps for a single key,
// hashing after every individual algorithm call (416 steps). Prints a full
// per-step breakdown table.
static void full_map_detail(tm_32_8& processor, uint32 key, int num_inputs)
{
    const int num_maps    = 26;
    const int algs_per_map = 16;
    const int num_steps   = num_maps * algs_per_map; // 416

    key_schedule schedule_data(key, key_schedule::ALL_MAPS);

    std::unordered_set<uint64_t> seen[num_steps];
    uint64_t unique_count[num_steps]    = {};
    uint64_t collision_count[num_steps] = {};
    uint8 state[128];

    printf("=== full_map_detail: Key 0x%08X, %d inputs, %d steps ===\n\n",
           key, num_inputs, num_steps);

    for (int n = 0; n < num_inputs; n++)
    {
        if (n % 10000 == 0 && n > 0)
        {
            double pct_done   = 100.0 * n / num_inputs;
            double mem_now_mb  = process_mem_mb();
            double mem_est_gb  = mem_now_mb * num_inputs / n / 1024.0;
            printf("Progress: %d / %d (%.1f%%)  Mem: %.1f MB  Est. final: %.2f GB\n",
                   n, num_inputs, pct_done, mem_now_mb, mem_est_gb);
        }

        processor.expand(key, (uint32)n);

        bool converged = false;
        for (int m = 0; m < num_maps && !converged; m++)
        {
            const key_schedule::key_schedule_entry& entry = schedule_data.entries[m];
            uint16 rng_seed        = ((uint16)entry.rng1 << 8) | entry.rng2;
            uint16 nibble_selector = entry.nibble_selector;

            processor.fetch_data(state);

            for (int i = 0; i < algs_per_map && !converged; i++)
            {
                unsigned char nibble       = (nibble_selector >> 15) & 0x01;
                nibble_selector            = nibble_selector << 1;
                unsigned char current_byte = state[i];
                if (nibble == 1)
                    current_byte = current_byte >> 4;
                unsigned char alg_id = (current_byte >> 1) & 0x07;

                processor.run_alg(alg_id, &rng_seed, 1);
                processor.fetch_data(state);

                int s = m * algs_per_map + i;
                uint64_t h = fnv1a_64(state, 128);

                if (seen[s].count(h))
                {
                    collision_count[s]++;
                    converged = true;
                }
                else
                {
                    seen[s].insert(h);
                    unique_count[s]++;
                }
            }
        }
    }

    printf("\n%-5s  %-4s  %-4s  %-10s  %-10s  %-10s  %-10s  %-12s\n",
           "Step", "Map", "Alg", "Reached", "Unique", "Collisions", "Unique%", "CumPruned%");
    printf("-----  ----  ----  ----------  ----------  ----------  ----------  ------------\n");

    uint64_t total_pruned = 0;
    for (int s = 0; s < num_steps; s++)
    {
        uint64_t reached  = unique_count[s] + collision_count[s];
        total_pruned     += collision_count[s];
        double unique_pct = reached > 0 ? 100.0 * unique_count[s] / reached : 0.0;
        double pruned_pct = 100.0 * total_pruned / num_inputs;
        printf("%-5d  %-4d  %-4d  %-10llu  %-10llu  %-10llu  %9.4f%%  %11.4f%%\n",
               s, s / algs_per_map, s % algs_per_map,
               reached, unique_count[s], collision_count[s], unique_pct, pruned_pct);
    }

    uint64_t completed = unique_count[num_steps - 1];
    printf("\nCompleted all %d steps: %llu / %d (%.2f%%)\n",
           num_steps, completed, num_inputs, 100.0 * completed / num_inputs);
    printf("Pruned early:           %llu / %d (%.2f%%)\n",
           total_pruned, num_inputs, 100.0 * total_pruned / num_inputs);
}

// Runs inputs in batches, clearing hash tables between each batch to keep memory
// bounded. Accumulates collision counts across all batches and prints the average
// prune rate at each step over the full run.
static void batched_map_detail(tm_32_8& processor, uint32 key, int batch_size, int num_batches)
{
    const int num_maps     = 26;
    const int algs_per_map = 16;
    const int num_steps    = num_maps * algs_per_map; // 416

    key_schedule schedule_data(key, key_schedule::ALL_MAPS);

    uint64_t total_collision_count[num_steps] = {};

    std::unordered_set<uint64_t> seen[num_steps];
    uint8 state[128];

    int total_inputs = batch_size * num_batches;

    printf("=== batched_map_detail: Key 0x%08X, %d batches x %d inputs = %d total ===\n\n",
           key, num_batches, batch_size, total_inputs);

    for (int b = 0; b < num_batches; b++)
    {
        printf("--- Batch %d / %d  (inputs %d - %d) ---\n",
               b + 1, num_batches, b * batch_size, (b + 1) * batch_size - 1);

        for (int s = 0; s < num_steps; s++)
            seen[s].clear();

        uint64_t batch_collision_count[num_steps] = {};

        for (int n = 0; n < batch_size; n++)
        {
            if (n % 100000 == 0 && n > 0)
            {
                uint64_t batch_pruned_so_far = 0;
                for (int s = 0; s < num_steps; s++)
                    batch_pruned_so_far += batch_collision_count[s];

                double mem_now      = process_mem_mb();
                double mem_est      = mem_now * batch_size / n / 1024.0;
                double prune_so_far = 100.0 * batch_pruned_so_far / n;
                printf("  Progress: %d / %d (%.1f%%)  Prune so far: %.3f%%  Mem: %.1f MB  Est. batch peak: %.2f GB\n",
                       n, batch_size, 100.0 * n / batch_size, prune_so_far, mem_now, mem_est);
            }

            processor.expand(key, (uint32)(b * batch_size + n));

            bool converged = false;
            for (int m = 0; m < num_maps && !converged; m++)
            {
                const key_schedule::key_schedule_entry& entry = schedule_data.entries[m];
                uint16 rng_seed        = ((uint16)entry.rng1 << 8) | entry.rng2;
                uint16 nibble_selector = entry.nibble_selector;

                processor.fetch_data(state);

                for (int i = 0; i < algs_per_map && !converged; i++)
                {
                    unsigned char nibble       = (nibble_selector >> 15) & 0x01;
                    nibble_selector            = nibble_selector << 1;
                    unsigned char current_byte = state[i];
                    if (nibble == 1)
                        current_byte = current_byte >> 4;
                    unsigned char alg_id = (current_byte >> 1) & 0x07;

                    processor.run_alg(alg_id, &rng_seed, 1);
                    processor.fetch_data(state);

                    int s      = m * algs_per_map + i;
                    uint64_t h = fnv1a_64(state, 128);

                    if (seen[s].count(h))
                    {
                        batch_collision_count[s]++;
                        converged = true;
                    }
                    else
                    {
                        seen[s].insert(h);
                    }
                }
            }
        }

        uint64_t batch_total_pruned = 0;
        for (int s = 0; s < num_steps; s++)
        {
            total_collision_count[s] += batch_collision_count[s];
            batch_total_pruned       += batch_collision_count[s];
        }
        printf("  Batch prune: %llu / %d (%.4f%%)\n\n",
               batch_total_pruned, batch_size, 100.0 * batch_total_pruned / batch_size);
    }

    printf("%-5s  %-4s  %-4s  %-14s  %-12s\n",
           "Step", "Map", "Alg", "AvgCollisions", "AvgCumPrune%");
    printf("-----  ----  ----  --------------  ------------\n");

    uint64_t cum_collisions = 0;
    for (int s = 0; s < num_steps; s++)
    {
        cum_collisions += total_collision_count[s];
        double avg_col     = (double)total_collision_count[s] / num_batches;
        double avg_cum_pct = 100.0 * cum_collisions / total_inputs;
        printf("%-5d  %-4d  %-4d  %14.2f  %11.4f%%\n",
               s, s / algs_per_map, s % algs_per_map, avg_col, avg_cum_pct);
    }

    printf("\nAvg total prune over %d batches: %.4f%%\n",
           num_batches, 100.0 * cum_collisions / total_inputs);
}

// Searches num_keys random keys for the one with the lowest prune rate up to
// stop_at_step (0-415, inclusive). Exits early from any key the moment its
// collision count already guarantees it can't beat the current best.
static void find_lowest_prune_key(tm_32_8& processor, int num_keys, int num_inputs, int stop_at_step)
{
    const int algs_per_map = 16;
    const int num_steps    = stop_at_step + 1;

    std::mt19937 key_gen(std::random_device{}());
    std::vector<std::unordered_set<uint64_t>> seen(num_steps);
    uint8 state[128];

    double best_prune_ratio  = 1.0; // 100% — updated as we find better keys
    uint32 best_key          = 0;
    int      keys_fully_run     = 0;
    int      keys_exited_early  = 0;
    uint64_t total_exit_input   = 0; // sum of input index at which each early exit fired
    uint64_t window_exit_input  = 0; // sum for the current window only

    printf("=== find_lowest_prune_key: %d keys, %d inputs each, up to step %d (map %d, alg %d) ===\n\n",
           num_keys, num_inputs, stop_at_step, stop_at_step / algs_per_map, stop_at_step % algs_per_map);

    for (int k = 0; k < num_keys; k++)
    {
        if (k % 1000 == 0)
        {
            double avg_exit_pct = keys_exited_early > 0
                ? 100.0 * total_exit_input / ((uint64_t)keys_exited_early * num_inputs)
                : 0.0;
            printf("Progress: %d / %d  Best: Key 0x%08X  Prune: %.4f%%  Early exits: %d  Avg exit point: %.1f%%\n",
                   k, num_keys, best_key, best_prune_ratio * 100.0, keys_exited_early, avg_exit_pct);
        }

        uint32 key = key_gen();
        key_schedule schedule_data(key, key_schedule::ALL_MAPS);

        for (int s = 0; s < num_steps; s++)
            seen[s].clear();

        uint64_t collisions = 0;
        bool exited_early   = false;
        int  n              = 0;

        for (; n < num_inputs; n++)
        {
            processor.expand(key, (uint32)n);

            bool converged = false;
            int  step      = 0;

            for (int m = 0; m <= stop_at_step / algs_per_map && !converged; m++)
            {
                const key_schedule::key_schedule_entry& entry = schedule_data.entries[m];
                uint16 rng_seed        = ((uint16)entry.rng1 << 8) | entry.rng2;
                uint16 nibble_selector = entry.nibble_selector;

                processor.fetch_data(state);

                int algs_this_map = (m == stop_at_step / algs_per_map)
                                    ? stop_at_step % algs_per_map + 1
                                    : algs_per_map;

                for (int i = 0; i < algs_this_map && !converged; i++, step++)
                {
                    unsigned char nibble       = (nibble_selector >> 15) & 0x01;
                    nibble_selector            = nibble_selector << 1;
                    unsigned char current_byte = state[i];
                    if (nibble == 1)
                        current_byte = current_byte >> 4;
                    unsigned char alg_id = (current_byte >> 1) & 0x07;

                    processor.run_alg(alg_id, &rng_seed, 1);
                    processor.fetch_data(state);

                    uint64_t h = fnv1a_64(state, 128);
                    if (seen[step].count(h))
                    {
                        collisions++;
                        converged = true;
                    }
                    else
                    {
                        seen[step].insert(h);
                    }
                }
            }

            // Assume all remaining inputs are unique (best case for this key).
            // The final prune rate is then at least collisions/num_inputs.
            // If that already meets or beats the best, this key can't win.
            if ((double)collisions / num_inputs >= best_prune_ratio)
            {
                exited_early = true;
                break;
            }
        }

        if (exited_early)
        {
            keys_exited_early++;
            total_exit_input  += (n + 1);
            window_exit_input += (n + 1);

            if (keys_exited_early % 5 == 0)
            {
                double avg_exit_pct = 100.0 * window_exit_input / (5ULL * num_inputs);
                printf("  [exit update] %d early exits so far  Avg exit point (last 5): %.1f%% of inputs\n",
                       keys_exited_early, avg_exit_pct);
                window_exit_input = 0;
            }
        }
        else
        {
            keys_fully_run++;
            double prune_ratio = (double)collisions / num_inputs;
            if (prune_ratio < best_prune_ratio)
            {
                best_prune_ratio = prune_ratio;
                best_key         = key;
                printf("  New best at key %d: 0x%08X  Prune: %.4f%%\n",
                       k, best_key, best_prune_ratio * 100.0);
            }
        }
    }

    printf("\n=== Results ===\n");
    printf("Best key:          0x%08X  Prune: %.4f%%\n", best_key, best_prune_ratio * 100.0);
    printf("Keys fully run:    %d\n", keys_fully_run);
    double avg_exit_pct = keys_exited_early > 0
        ? 100.0 * total_exit_input / ((uint64_t)keys_exited_early * num_inputs)
        : 0.0;
    printf("Keys exited early: %d  (%.1f%%)  Avg exit point: %.1f%% of inputs\n",
           keys_exited_early, 100.0 * keys_exited_early / num_keys, avg_exit_pct);
}

int main()
{
    RNG     rng;
    tm_32_8 processor(&rng);

    // --- Experiment 1: sweep many random keys through the first map ---
    //sweep_keys(processor, 500, 100000);

    // --- Experiment 2: drill into a specific interesting key ---
    // Paste keys found from sweep_keys / find_lowest_prune_key here.
    // 0xE6B972B6  Prune: 12.0400%
    //full_map_detail(processor, 0x2CA5B42D, 4000000);

    // --- Experiment 4: batched run — hash tables cleared between batches ---
    // batched_map_detail(processor, key, batch_size, num_batches)
    //batched_map_detail(processor, 0x2CA5B42D, 1000000, 32); 
    batched_map_detail(processor, 0xE6B972B6, 1000000, 32);

    // --- Experiment 3: find the key with the lowest prune rate up to a given step ---
    // Step range: 0-415 (step = map * 16 + alg_within_map)
    //find_lowest_prune_key(processor, 100000, 100000, 64);  // first map only (step 15)
    //find_lowest_prune_key(processor, 100000, 100000, 415); // all 26 maps

    return 0;
}
