-- run_key_schedule_alg.lua
--
-- Tests individual Stage-9 ("key schedule") 4-byte algorithm routines in
-- isolation. The 8 algorithms are routine IDs $27..$2E in the $EE54
-- dispatcher.
--
-- Each TMTV inputs file (test_inputs/key_schedule_alg_N.inputs.bin where
-- N = 0..7) holds inputs for one specific algorithm, with header subtype = N.
-- Per record:
--   in:  [4] state_in + [1] map_id
--   out: [4] state_in + [1] map_id + [4] state_out
--
-- Approach (analog of run_wc_alg):
--   • Anchor on F03162_map_exit_00 (Stage 9 fires naturally there).
--   • Hook on $8288 (Stage 9's jsr LEE6B in the dispatch path) to:
--       - inject state4 into $0191..$0194,
--       - inject map_id into current_map ($00F3),
--       - override A = $27 + alg_id to force the algorithm we're testing.
--     The natural map_flags / $0150 conditions for map $00 cause this JSR
--     to fire; once it does, our injected state + override take effect.
--   • Hook on $828B (LDA $EC — first instr after the JSR returns) to
--     capture state4. Done with this test record.

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
local ALG_IDS           = nil    -- nil = all 8 (0..7); list = subset

-- ============================================================================
-- Constants
-- ============================================================================
local STATE4_BASE     = 0x0191       -- $0191..$0194
local CURRENT_MAP     = 0x00F3       -- map_id

local STAGE9_DISPATCH = 0x8288       -- jsr LEE6B with A in $27..$2E
local STAGE9_POST     = 0x828B       -- LDA $EC — first instr after JSR returns

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
local pending_state4   = nil
local pending_map_id   = nil
local pending_alg_id   = nil
local capture_armed    = false
local captured_state4  = nil

-- ============================================================================
-- Hooks
-- ============================================================================

-- $8288: just before jsr LEE6B fires for Stage 9. A holds the picked alg ID.
-- Override A to test alg, inject state4 + map_id so the alg sees our values.
memory.registerexecute(STAGE9_DISPATCH, function()
    if pending_alg_id == nil then return end
    lib.write_range(STATE4_BASE, pending_state4)
    memory.writebyte(CURRENT_MAP, pending_map_id)
    memory.setregister("a", 0x27 + pending_alg_id)
    pending_alg_id = nil
end)

-- $828B: just after the alg's RTS — state4 has been mutated.
memory.registerexecute(STAGE9_POST, function()
    if not capture_armed then return end
    captured_state4 = lib.read_range(STATE4_BASE, 4)
    capture_armed = false
end)

-- ============================================================================
-- Per-record runner
-- ============================================================================
local function run_one(state_in, map_id, alg_id)
    savestate.load(anchor_obj)

    pending_state4   = state_in
    pending_map_id   = map_id
    pending_alg_id   = alg_id
    capture_armed    = true
    captured_state4  = nil

    local cap_frame = emu.framecount() + MAX_LEAD_FRAMES
    while capture_armed and emu.framecount() < cap_frame do
        emu.frameadvance()
    end
    if capture_armed then
        capture_armed = false
        return nil, "$828B capture hook did not fire"
    end
    return captured_state4
end

-- ============================================================================
-- Process one inputs file
-- ============================================================================
local function process_file(alg_id)
    local in_path  = lib.test_inputs_dir  .. string.format("key_schedule_alg_%d.inputs.bin", alg_id)
    local out_path = lib.test_outputs_dir .. string.format("key_schedule_alg_%d.bin",        alg_id)

    local reader, err = lib.open_inputs(in_path, 0x02, alg_id, 5)
    if not reader then
        print(string.format("[key_schedule_alg %d] SKIP: %s", alg_id, err))
        return
    end

    local total = reader.record_count
    if MAX_TESTS and MAX_TESTS < total then total = MAX_TESTS end

    local out_f = lib.open_outputs(out_path, 0x02, alg_id, 9, total, reader.shared_count)
    if not out_f then
        print(string.format("[key_schedule_alg %d] ERROR opening output", alg_id))
        lib.close_inputs(reader)
        return
    end

    print(string.format("# === key_schedule_alg %d ===", alg_id))
    print(string.format("#   inputs:  %s  (records=%d)", in_path, reader.record_count))
    print(string.format("#   outputs: %s", out_path))
    if MAX_TESTS then print(string.format("#   MAX_TESTS=%d", MAX_TESTS)) end

    local start_time = os.time()
    for test_idx = 1, total do
        local rec = lib.read_record(reader)
        if not rec or #rec < 5 then
            print(string.format("#   short read at %d/%d", test_idx, total))
            break
        end
        local state_in = string.sub(rec, 1, 4)
        local map_id   = string.byte(rec, 5)

        local state_out, perr = run_one(state_in, map_id, alg_id)
        if not state_out then
            print(string.format("#   test %d/%d FAILED: %s", test_idx, total, perr))
            break
        end

        out_f:write(rec)             -- echo input
        out_f:write(state_out)       -- 4-byte mutated state4

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
    print(string.format("# key_schedule_alg %d done. %d/%d records in %ds (%.2f rec/s)",
        alg_id, total, total, elapsed, rate))
end

-- ============================================================================
-- Main
-- ============================================================================
local ids = ALG_IDS or {0, 1, 2, 3, 4, 5, 6, 7}
for _, alg_id in ipairs(ids) do
    if alg_id >= 0 and alg_id <= 7 then
        process_file(alg_id)
    else
        print(string.format("[key_schedule_alg] skipping invalid alg_id %d", alg_id))
    end
end

print("# all key_schedule_alg files processed")
