-- run_key_schedule_dispatch.lua
--
-- Tests the full Stage-9 dispatcher path at $0:8255: gate checks +
-- selector formula + chosen algorithm execution. For each (state, map_id)
-- pair, it captures which routine the dispatcher selected and the
-- resulting state4 mutation.
--
-- TMTV format:
--   in:  [4] state_in + [1] map_id                       (5 bytes)
--   out: [4] state_in + [1] map_id + [4] state_out
--          + [1] ran_flag + [1] routine_id              (11 bytes)
--   test_type = 0x01
--
-- Approach (analog of run_key_schedule_alg, but DON'T override the alg ID
-- — let the dispatcher pick naturally):
--   • Anchor on F03162_map_exit_00 (Stage 8 fires naturally there).
--   • Hook on $8255 (Stage 8 entry):
--       - inject state4 into $0191..$0194,
--       - inject map_id into $00F3 AND into the X register (so the TXA
--         at $825C uses our map_id),
--       - clear $0150[map>>3] so gate-2 always passes,
--       - clear $EC bit 5 so we can detect whether the alg's tail-set fires,
--       - override PC = $825C to bypass gate-1 entirely (we want every
--         map_id to test the dispatch path, not just gameplay-flagged ones).
--   • Hook on $EE6B (dispatcher entry): if a routine_id is still pending,
--     latch A as the captured routine_id (first fire wins).
--   • Hook on $828B (post-dispatch convergence): capture state4, ran flag,
--     and the latched routine_id. Done with this record.

local lib = dofile((function()
    local src = debug.getinfo(1, "S").source
    if src:sub(1, 1) == "@" then src = src:sub(2) end
    return (src:match("(.*[/\\])") or "") .. "lib_test.lua"
end)())

-- ============================================================================
-- Configuration
-- ============================================================================
local PROGRESS_INTERVAL = 5000
local MAX_TESTS         = nil
local MAX_LEAD_FRAMES   = 30

-- ============================================================================
-- Constants
-- ============================================================================
local STATE4_BASE       = 0x0191
local CURRENT_MAP       = 0x00F3
local BITMAP_BASE       = 0x0150        -- $0150,X with X = map>>3
local EC_ADDR           = 0x00EC

local STAGE8_ENTRY_PC   = 0x8255        -- LDX current_map
local STAGE8_POSTGATE1  = 0x825C        -- TXA — first instr after BPL gate-1
local DISPATCHER_PC     = 0xEE6B        -- A holds routine ID at this point
local CONVERGE_PC       = 0x828B        -- LDA $EC — post-dispatch convergence

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
local capture_armed    = false
local captured_routine = 0xFF       -- $FF = no dispatch fired
local captured_state4  = nil
local captured_ran     = nil

-- ============================================================================
-- Hooks
-- ============================================================================

-- $EE6B: latch the routine ID on the first dispatch after arming.
memory.registerexecute(DISPATCHER_PC, function()
    if not capture_armed then return end
    if captured_routine ~= 0xFF then return end       -- already captured
    captured_routine = memory.getregister("a")
end)

-- $828B: post-dispatch convergence — capture state4, ran flag, routine id.
memory.registerexecute(CONVERGE_PC, function()
    if not capture_armed then return end
    captured_state4 = lib.read_range(STATE4_BASE, 4)
    local ec = memory.readbyte(EC_ADDR)
    captured_ran    = (math.floor(ec / 32) % 2 == 1) and 1 or 0   -- bit 5
    capture_armed   = false
end)

-- ============================================================================
-- Per-record runner
-- ============================================================================
local function run_one(state_in, map_id)
    savestate.load(anchor_obj)

    -- Inject test inputs directly (Lua-level, before frame-advance).
    lib.write_range(STATE4_BASE, state_in)
    memory.writebyte(CURRENT_MAP, map_id)
    -- Pre-clear gate-2's bitmap byte covering this map.
    memory.writebyte(BITMAP_BASE + math.floor(map_id / 8), 0x00)
    -- Pre-clear $EC bit 5 so we can detect whether the alg sets it.
    local ec = memory.readbyte(EC_ADDR)
    memory.writebyte(EC_ADDR, ec - (math.floor(ec / 32) % 2) * 32)
    -- Trampoline: jump CPU directly to $825C (post-gate-1), with X = map_id
    -- so the TXA there picks up our value.
    memory.setregister("x", map_id)
    memory.setregister("pc", STAGE8_POSTGATE1)

    captured_routine  = 0xFF
    captured_state4   = nil
    captured_ran      = nil
    capture_armed     = true

    local cap_frame = emu.framecount() + MAX_LEAD_FRAMES
    while capture_armed and emu.framecount() < cap_frame do
        emu.frameadvance()
    end
    if capture_armed then
        capture_armed = false
        return nil, "$828B convergence hook did not fire"
    end
    return captured_state4, captured_ran, captured_routine
end

-- ============================================================================
-- Process the inputs file
-- ============================================================================
local in_path  = lib.test_inputs_dir  .. "key_schedule_dispatch.inputs.bin"
local out_path = lib.test_outputs_dir .. "key_schedule_dispatch.bin"

local reader, err = lib.open_inputs(in_path, 0x01, 0, 5)
if not reader then
    print("ERROR: " .. err)
    return
end

local total = reader.record_count
if MAX_TESTS and MAX_TESTS < total then total = MAX_TESTS end

local out_f = lib.open_outputs(out_path, 0x01, 0, 11, total, reader.shared_count)
if not out_f then
    print("ERROR: failed to open outputs")
    lib.close_inputs(reader)
    return
end

print(string.format("# === key_schedule_dispatch ==="))
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

    local state_out, ran_flag, routine_id = run_one(state_in, map_id)
    if not state_out then
        print(string.format("#   test %d/%d FAILED: %s", test_idx, total, ran_flag))
        break
    end

    out_f:write(rec)
    out_f:write(state_out)
    out_f:write(string.char(ran_flag))
    out_f:write(string.char(routine_id))

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
print(string.format("# all done. %d/%d records in %ds (%.2f rec/s)",
    total, total, elapsed, rate))
