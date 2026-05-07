-- run_key_schedule_all_maps.lua
--
-- Threads a 4-byte input state through all 28 key-schedule events in the
-- chain, capturing the post-Stage-9 state4 after each one. Output: the
-- input plus 27 emitted intermediate states (the silent rocket_entry
-- event's state passes through but isn't emitted).
--
-- TMTV format:
--   in:  [4] state_in                                          (4 bytes)
--   out: [4] state_in + [27 × 4] state_after_event_N         (112 bytes)
--   test_type = 0x03
--
-- Approach:
--   For each event in order:
--     • savestate.load(event_savestate)
--     • inject carried-forward state4 into $0191..$0194
--     • frame-advance — Stage 9's natural dispatch runs and mutates state4
--     • capture post-dispatch state4 at $828B (convergence)
--     • carry forward to next event
--
-- This is the state4-only analog of run_all_maps. Stage 10's 128-byte
-- mutation runs naturally (we don't override anything related to it) but
-- doesn't affect state4, so we ignore its output.

local lib = dofile((function()
    local src = debug.getinfo(1, "S").source
    if src:sub(1, 1) == "@" then src = src:sub(2) end
    return (src:match("(.*[/\\])") or "") .. "lib_test.lua"
end)())

-- ============================================================================
-- Configuration
-- ============================================================================
local INPUTS_FILE       = lib.test_inputs_dir  .. "key_schedule_all_maps.inputs.bin"
local OUTPUTS_FILE      = lib.test_outputs_dir .. "key_schedule_all_maps.bin"
local PROGRESS_INTERVAL = 50
local MAX_TESTS         = nil
local MAX_LEAD_FRAMES   = 60       -- per-event safety cap

-- ============================================================================
-- Constants
-- ============================================================================
local STATE4_BASE  = 0x0191
local POST_ALG_PC  = 0xEE8D        -- universal "dispatched alg just RTS'd" hook;
                                   -- catches Stage 9 algs whether they came via
                                   -- the normal $8288 path or a hardcoded dispatch.
local SUBTYPE      = 27            -- chain length emitted

-- Each entry: { frame, emit, silent }.
--   emit=false   → the state passes through but isn't written to output
--   silent=true  → use a time-based wait instead of the $828B hook (the
--                  event's Stage 9 fires via a sprite-driven dispatch path
--                  that doesn't pass through the normal convergence point)
local EVENTS = {
    {  3161, true,  false },   -- map_exit_00
    {  8941, true,  false },   -- map_exit_02
    {  9352, true,  false },   -- map_exit_05
    {  9549, true,  false },   -- map_exit_04
    {  9744, true,  false },   -- map_exit_03
    { 17289, true,  false },   -- map_exit_1D
    { 17484, true,  false },   -- map_exit_1C
    { 18793, true,  false },   -- map_exit_1E
    { 18829, false, true  },   -- rocket_entry (silent, sprite-driven $2D dispatch)
    { 19367, true,  false },   -- map_exit_1B
    { 20555, true,  false },   -- map_exit_07
    { 21804, true,  false },   -- map_exit_08
    { 22733, true,  false },   -- car_entry (4-byte half of split event)
    { 25027, true,  false },   -- map_exit_09
    { 31961, true,  false },   -- map_exit_0C
    { 33681, true,  false },   -- map_exit_20
    { 34249, true,  false },   -- map_exit_21
    { 34841, true,  false },   -- map_exit_22
    { 37679, true,  false },   -- machine_part (4-byte half)
    { 41860, true,  false },   -- map_exit_23
    { 43222, true,  false },   -- map_exit_24
    { 49666, true,  false },   -- map_exit_25
    { 50268, true,  false },   -- map_exit_26
    { 54196, true,  false },   -- map_exit_0E
    { 54666, true,  false },   -- map_exit_0F
    { 58654, true,  false },   -- map_exit_10
    { 61565, true,  false },   -- map_exit_12
    { 63885, true,  false },   -- map_exit_11
}

local CHAIN_LENGTH = 0
for _, e in ipairs(EVENTS) do if e[2] then CHAIN_LENGTH = CHAIN_LENGTH + 1 end end
local RECORD_SIZE_OUT = 4 + CHAIN_LENGTH * 4   -- 4 + 27*4 = 112

local SILENT_POST_LEAD = 5    -- frames past silent event before snapshotting

-- ============================================================================
-- Pre-load savestate objects (stored in e[4] since e[3] is the silent flag)
-- ============================================================================
for i, e in ipairs(EVENTS) do
    local frame = e[1]
    local entry = lib.savestates[frame]
    if not entry then
        error("no savestate registered at frame " .. frame)
    end
    local f = io.open(entry.file, "rb")
    if not f then error("savestate file not found: " .. entry.file) end
    f:close()
    e[4] = savestate.create(entry.file)
end
print(string.format("[run_key_schedule_all_maps] %d savestate objects pre-loaded", #EVENTS))

-- ============================================================================
-- Capture hook: $828B (post-dispatch convergence) → snapshot state4.
-- ============================================================================
local capture_armed   = false
local captured_state4 = nil

memory.registerexecute(POST_ALG_PC, function()
    if not capture_armed then return end
    captured_state4 = lib.read_range(STATE4_BASE, 4)
    capture_armed   = false
end)

-- ============================================================================
-- Run one event:
--   • Normal events: hook on $828B (convergence) captures post-Stage-9 state.
--   • Silent events: sprite-driven dispatch doesn't pass through $828B; do
--     a fixed time-based wait and read state4 directly.
-- ============================================================================
local function run_event(ss_obj, state4_in, frame, silent)
    savestate.load(ss_obj)
    lib.write_range(STATE4_BASE, state4_in)

    if silent then
        local target = frame + SILENT_POST_LEAD
        while emu.framecount() < target do emu.frameadvance() end
        return lib.read_range(STATE4_BASE, 4)
    end

    captured_state4 = nil
    capture_armed   = true
    local cap_frame = emu.framecount() + MAX_LEAD_FRAMES
    while capture_armed and emu.framecount() < cap_frame do
        emu.frameadvance()
    end
    if capture_armed then
        capture_armed = false
        return nil
    end
    return captured_state4
end

-- ============================================================================
-- Run a full chain on one 4-byte input
-- ============================================================================
local function run_chain(state4_in)
    local state4 = state4_in
    local emitted = {}
    for i, e in ipairs(EVENTS) do
        state4 = run_event(e[4], state4, e[1], e[3])
        if not state4 then
            return nil, string.format("event %d (frame %d) capture failed", i, e[1])
        end
        if e[2] then
            emitted[#emitted + 1] = state4
        end
    end
    return emitted
end

-- ============================================================================
-- Main loop
-- ============================================================================
local reader, err = lib.open_inputs(INPUTS_FILE, 0x03, SUBTYPE, 4)
if not reader then
    print("ERROR: " .. err)
    return
end

local total = reader.record_count
if MAX_TESTS and MAX_TESTS < total then total = MAX_TESTS end

local out_f = lib.open_outputs(OUTPUTS_FILE, 0x03, SUBTYPE, RECORD_SIZE_OUT, total, reader.shared_count)
if not out_f then
    print("ERROR: failed to open outputs")
    lib.close_inputs(reader)
    return
end

print(string.format("# === key_schedule_all_maps (state4 chain through 28 events, 27 emit) ==="))
print(string.format("#   inputs:  %s  (records=%d)", INPUTS_FILE, reader.record_count))
print(string.format("#   outputs: %s", OUTPUTS_FILE))
if MAX_TESTS then print(string.format("#   MAX_TESTS=%d", MAX_TESTS)) end

local start_time = os.time()
for test_idx = 1, total do
    local state_in = lib.read_record(reader)
    if not state_in or #state_in < 4 then
        print(string.format("#   short read at %d/%d", test_idx, total))
        break
    end
    local emitted, perr = run_chain(state_in)
    if not emitted then
        print(string.format("#   test %d/%d FAILED: %s", test_idx, total, perr))
        break
    end
    out_f:write(state_in)
    for _, s in ipairs(emitted) do out_f:write(s) end
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
