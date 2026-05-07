-- run_wc_alg.lua
--
-- Tests Stage-10 ("wc") algorithm routines, both individually and in
-- N-algorithm chains. Handles two TMTV test types:
--
--   test_type = 0x05  (single algorithm)
--     subtype = alg_id (0..7)
--     in:  [2] rng_in + [128] buf_in
--     out: in + [2] rng_out + [128] buf_out
--     Files: wc_alg_0.inputs.bin .. wc_alg_7.inputs.bin
--
--   test_type = 0x06  (multi-algorithm chain)
--     subtype = N (chain length, ≥1)
--     in:  [2] rng_in + [128] buf_in + [N] alg_ids
--     out: in + [2] rng_out + [128] buf_out
--     Files: wc_alg_multi_2/3/4/8/11/16.inputs.bin
--
-- Approach:
--   • Anchor on F03161_map_exit_00 (Stage 10 fires naturally there).
--   • Hook on $82C3 (round 0, X=0): inject buf128 + RNG once per record.
--   • Hook on $82D4 (round k's jsr LEE6B): override A = $1F + alg_chain[k]
--     for each round we want to control. Single-alg case has chain=[id];
--     N-alg case has chain=alg_ids[1..N].
--   • Hook on $82D7 (LDX $65 — first instr after the JSR returns): once
--     N RTSes have been observed, capture post-chain buf128 + RNG and
--     stop the per-record loop.
--
-- The natural Stage 10 loop runs 16 rounds; we don't care about rounds
-- past N because savestate.load resets between records.

local lib = dofile((function()
    local src = debug.getinfo(1, "S").source
    if src:sub(1, 1) == "@" then src = src:sub(2) end
    return (src:match("(.*[/\\])") or "") .. "lib_test.lua"
end)())

-- ============================================================================
-- Configuration
-- ============================================================================
local PROGRESS_INTERVAL = 5000
local MAX_TESTS         = nil    -- nil = process all records
local MAX_LEAD_FRAMES   = 30     -- safety cap on per-test frame-advance

-- Filter what to run. nil for each = run everything in that category.
local SINGLE_ALG_IDS    = nil    -- e.g. {0} to run only wc_alg_0
local MULTI_LENGTHS     = nil    -- e.g. {2, 4} to run only wc_alg_multi_2/4

-- ============================================================================
-- Constants
-- ============================================================================
local BUFFER_BASE     = 0x0200
local BUFFER_LEN      = 128
local RNG_LO          = 0x0436
local RNG_HI          = 0x0437

local STAGE10_R0_LDA  = 0x82C3   -- LDA $0200,X (X=0 ⇒ round 0)
local STAGE10_JSR     = 0x82D4   -- JSR LEE6B (alg dispatch); A holds routine ID
local STAGE10_POST    = 0x82D7   -- LDX $65 — first instr after JSR returns

-- ============================================================================
-- Anchor savestate
-- ============================================================================
local ANCHOR_FRAME = 3161
local anchor_obj
do
    local path = lib.savestates[ANCHOR_FRAME].file
    local f = io.open(path, "rb")
    if not f then
        print("ERROR: anchor savestate not found: " .. path)
        return
    end
    f:close()
    anchor_obj = savestate.create(path)
end

-- ============================================================================
-- Per-record state
-- ============================================================================
local pending_buf      = nil   -- 128-byte buf to inject at $82C3 X=0
local pending_rng_lo   = nil
local pending_rng_hi   = nil
local alg_chain        = nil   -- table of alg ids (length N), 1-indexed
local chain_idx        = 0     -- next alg to dispatch (0..N-1, Lua-translated to alg_chain[chain_idx+1])
local capture_armed    = false
local captured_buf     = nil
local captured_rng_lo  = nil
local captured_rng_hi  = nil

-- ============================================================================
-- Hooks
-- ============================================================================

-- Round 0, X=0: inject our test buf + RNG into RAM. Fires once per record.
memory.registerexecute(STAGE10_R0_LDA, function()
    if not pending_buf then return end
    if memory.getregister("x") ~= 0 then return end
    lib.write_range(BUFFER_BASE, pending_buf)
    memory.writebyte(RNG_LO, pending_rng_lo)
    memory.writebyte(RNG_HI, pending_rng_hi)
    pending_buf = nil
end)

-- JSR LEE6B in Stage 10: override A with the next chained alg id, if any.
memory.registerexecute(STAGE10_JSR, function()
    if not alg_chain then return end
    if chain_idx >= #alg_chain then return end
    memory.setregister("a", 0x1F + alg_chain[chain_idx + 1])
end)

-- Just after the JSR returns: count this dispatch. Once we've run all N,
-- capture and disarm.
memory.registerexecute(STAGE10_POST, function()
    if not alg_chain then return end
    chain_idx = chain_idx + 1
    if chain_idx >= #alg_chain and capture_armed then
        captured_buf    = lib.read_range(BUFFER_BASE, BUFFER_LEN)
        captured_rng_lo = memory.readbyte(RNG_LO)
        captured_rng_hi = memory.readbyte(RNG_HI)
        capture_armed   = false
        alg_chain       = nil
    end
end)

-- ============================================================================
-- Per-record runner
-- ============================================================================
local function run_one(rng_in_lo, rng_in_hi, buf_in, alg_ids)
    savestate.load(anchor_obj)

    pending_buf      = buf_in
    pending_rng_lo   = rng_in_lo
    pending_rng_hi   = rng_in_hi
    alg_chain        = alg_ids
    chain_idx        = 0
    capture_armed    = true
    captured_buf     = nil
    captured_rng_lo  = nil
    captured_rng_hi  = nil

    local cap_frame = emu.framecount() + MAX_LEAD_FRAMES
    while capture_armed and emu.framecount() < cap_frame do
        emu.frameadvance()
    end
    if capture_armed then
        capture_armed = false
        alg_chain     = nil
        return nil, "capture hook ($82D7) did not fire within MAX_LEAD_FRAMES"
    end
    return captured_buf, captured_rng_lo, captured_rng_hi
end

-- ============================================================================
-- Process one single-algorithm file (test_type 0x05)
-- ============================================================================
local function process_single(alg_id)
    local in_path  = lib.test_inputs_dir  .. string.format("wc_alg_%d.inputs.bin", alg_id)
    local out_path = lib.test_outputs_dir .. string.format("wc_alg_%d.bin",        alg_id)

    local reader, err = lib.open_inputs(in_path, lib.TEST_TYPE_WC_ALG, alg_id, 130)
    if not reader then
        print(string.format("[wc_alg %d] SKIP: %s", alg_id, err))
        return
    end

    local total = reader.record_count
    if MAX_TESTS and MAX_TESTS < total then total = MAX_TESTS end

    local out_f = lib.open_outputs(out_path, lib.TEST_TYPE_WC_ALG, alg_id, 260, total, reader.shared_count)
    if not out_f then
        print(string.format("[wc_alg %d] ERROR opening output", alg_id))
        lib.close_inputs(reader)
        return
    end

    print(string.format("# === wc_alg %d (single algorithm) ===", alg_id))
    print(string.format("#   inputs:  %s  (records=%d)", in_path, reader.record_count))
    print(string.format("#   outputs: %s", out_path))
    if MAX_TESTS then print(string.format("#   MAX_TESTS=%d", MAX_TESTS)) end

    local chain_one = { alg_id }    -- single-alg = chain of length 1
    local start_time = os.time()
    for test_idx = 1, total do
        local rec = lib.read_record(reader)
        if not rec or #rec < 130 then
            print(string.format("#   short read at %d/%d", test_idx, total))
            break
        end
        local rng_lo = string.byte(rec, 1)
        local rng_hi = string.byte(rec, 2)
        local buf_in = string.sub(rec, 3, 130)

        local buf_out, rng_out_lo, rng_out_hi = run_one(rng_lo, rng_hi, buf_in, chain_one)
        if not buf_out then
            print(string.format("#   test %d/%d FAILED: %s", test_idx, total, rng_out_lo))
            break
        end

        out_f:write(rec)
        out_f:write(string.char(rng_out_lo, rng_out_hi))
        out_f:write(buf_out)

        if PROGRESS_INTERVAL > 0 and test_idx % PROGRESS_INTERVAL == 0 then
            local elapsed = os.time() - start_time
            local rate    = elapsed > 0 and (test_idx / elapsed) or 0
            local eta     = rate > 0 and ((total - test_idx) / rate) or 0
            print(string.format("#   progress: %d/%d  %.2f rec/s  ETA %ds",
                test_idx, total, rate, eta))
        end
    end

    lib.close_inputs(reader)
    out_f:close()
    local elapsed = os.time() - start_time
    local rate    = elapsed > 0 and (total / elapsed) or 0
    print(string.format("# wc_alg %d done. %d/%d records in %ds (%.2f rec/s)",
        alg_id, total, total, elapsed, rate))
end

-- ============================================================================
-- Process one multi-algorithm chain file (test_type 0x06)
-- ============================================================================
local function process_multi(N)
    local in_path  = lib.test_inputs_dir  .. string.format("wc_alg_multi_%d.inputs.bin", N)
    local out_path = lib.test_outputs_dir .. string.format("wc_alg_multi_%d.bin",        N)
    local in_size  = 130 + N
    local out_size = 260 + N

    local reader, err = lib.open_inputs(in_path, lib.TEST_TYPE_WC_ALG_MULTI, N, in_size)
    if not reader then
        print(string.format("[wc_alg_multi %d] SKIP: %s", N, err))
        return
    end

    local total = reader.record_count
    if MAX_TESTS and MAX_TESTS < total then total = MAX_TESTS end

    local out_f = lib.open_outputs(out_path, lib.TEST_TYPE_WC_ALG_MULTI, N, out_size, total, reader.shared_count)
    if not out_f then
        print(string.format("[wc_alg_multi %d] ERROR opening output", N))
        lib.close_inputs(reader)
        return
    end

    print(string.format("# === wc_alg_multi %d (chain of %d algorithms) ===", N, N))
    print(string.format("#   inputs:  %s  (records=%d)", in_path, reader.record_count))
    print(string.format("#   outputs: %s", out_path))
    if MAX_TESTS then print(string.format("#   MAX_TESTS=%d", MAX_TESTS)) end

    local start_time = os.time()
    for test_idx = 1, total do
        local rec = lib.read_record(reader)
        if not rec or #rec < in_size then
            print(string.format("#   short read at %d/%d", test_idx, total))
            break
        end
        local rng_lo = string.byte(rec, 1)
        local rng_hi = string.byte(rec, 2)
        local buf_in = string.sub(rec, 3, 130)
        local chain  = {}
        for i = 1, N do
            chain[i] = string.byte(rec, 130 + i)
        end

        local buf_out, rng_out_lo, rng_out_hi = run_one(rng_lo, rng_hi, buf_in, chain)
        if not buf_out then
            print(string.format("#   test %d/%d FAILED: %s", test_idx, total, rng_out_lo))
            break
        end

        out_f:write(rec)
        out_f:write(string.char(rng_out_lo, rng_out_hi))
        out_f:write(buf_out)

        if PROGRESS_INTERVAL > 0 and test_idx % PROGRESS_INTERVAL == 0 then
            local elapsed = os.time() - start_time
            local rate    = elapsed > 0 and (test_idx / elapsed) or 0
            local eta     = rate > 0 and ((total - test_idx) / rate) or 0
            print(string.format("#   progress: %d/%d  %.2f rec/s  ETA %ds",
                test_idx, total, rate, eta))
        end
    end

    lib.close_inputs(reader)
    out_f:close()
    local elapsed = os.time() - start_time
    local rate    = elapsed > 0 and (total / elapsed) or 0
    print(string.format("# wc_alg_multi %d done. %d/%d records in %ds (%.2f rec/s)",
        N, total, total, elapsed, rate))
end

-- ============================================================================
-- Main
-- ============================================================================
local DEFAULT_SINGLE = {0, 1, 2, 3, 4, 5, 6, 7}
local DEFAULT_MULTI  = {2, 3, 4, 8, 11, 16}

for _, alg_id in ipairs(SINGLE_ALG_IDS or DEFAULT_SINGLE) do
    if alg_id >= 0 and alg_id <= 7 then process_single(alg_id) end
end

for _, N in ipairs(MULTI_LENGTHS or DEFAULT_MULTI) do
    if N >= 1 then process_multi(N) end
end

print("# all wc_alg files processed")
