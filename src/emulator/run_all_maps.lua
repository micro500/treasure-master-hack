-- run_all_maps.lua
--
-- Runs the full password-screen processing chain for each 8-byte input
-- key in test_inputs/all_maps.inputs.bin and writes the final
-- 128-byte buffer per record to test_outputs/all_maps.bin.
--
-- Pipeline per record:
--   1. Stage A: $B112 expansion (8 bytes → 128 bytes at $0200..$027F).
--      Driven from the F02550_expansion savestate, hook on $B112.
--   2. Stage B: 28 in-game key-mixing events (Stage 9 4-byte mutation +
--      Stage 10 16-round chain mutation per event), threaded via per-event
--      savestates. Hook on $82C3 (LDA $0200,X) at X=0 to inject the
--      carried-forward buf128 into $0200..$027F right before Stage 10
--      reads it.
--   3. Final result = $0200..$027F after the last event.
--
-- 27 of 28 events emit a 128-byte output (event "rocket_entry" is silent
-- — its 128-byte step is overwritten before anything reads it). Output
-- record format: 8-byte key + 27 × (4-byte state4 + 128-byte buffer)
-- + 4-byte final state4 + 128-byte final buffer.

local lib = dofile((function()
    local src = debug.getinfo(1, "S").source
    if src:sub(1, 1) == "@" then src = src:sub(2) end
    return (src:match("(.*[/\\])") or "") .. "lib_test.lua"
end)())

-- ============================================================================
-- Configuration
-- ============================================================================
local INPUTS_FILE       = lib.test_inputs_dir  .. "all_maps.inputs.bin"
local OUTPUTS_FILE      = lib.test_outputs_dir .. "all_maps.bin"
local DEBUG_FILE        = nil  -- e.g. lib.test_outputs_dir .. "all_maps_debug.txt"
local PROGRESS_INTERVAL = 100
local MAX_TESTS         = nil  -- nil = process all records; cap to verify a subset

local POST_LEAD         = 5    -- frames past chain completion to settle route $1E
local MAX_LEAD          = 200  -- safety cap on per-event frame-advance

-- ============================================================================
-- Constants
-- ============================================================================
local STATE4_BASE       = 0x0191    -- $0191..$0194
local BUFFER_BASE       = 0x0200    -- $0200..$027F
local BUFFER_LEN        = 128

local EXPANSION_PC      = 0xB112    -- expansion entry: reads $0200..$0207, writes $0200..$027F
local STAGE10_FIRST_PC  = 0x82C3    -- LDA $0200,X — fires once per round; X==0 ⇒ round 0
local DISPATCHER_PC     = 0xEE6B    -- alg dispatcher: A in $1F..$26 = Stage 10 alg
local STAGE10_DONE_PC   = 0x82E8    -- jsr LEE54 (route $1E call). Reached only via 16-round loop exit, NOT via the BEQ-skip path at $828D. So this PC uniquely signals "Stage 10 just finished, $0200..$027F has the final buffer".

local SUBTYPE           = 27
local RECORD_SIZE_IN    = 8
local RECORD_SIZE_OUT   = 8 + BUFFER_LEN    -- key + final 128-byte buffer

-- ============================================================================
-- Chain definition: every event in chronological order, by frame
--
-- Each entry has:
--   type    : "normal" | "silent" | "delayed"
--   frame   : savestate frame for the event start (= main savestate)
--   frame2  : (delayed only) savestate frame for the 128-byte sub-step
--   emit    : whether to emit a (state4, buf128) record for this event
-- ============================================================================
local CHAIN = {
    -- 7 events leading up to the first map-$1E
    { type = "normal",  frame =  3161, emit = true },
    { type = "normal",  frame =  8941, emit = true },
    { type = "normal",  frame =  9352, emit = true },
    { type = "normal",  frame =  9549, emit = true },
    { type = "normal",  frame =  9744, emit = true },
    { type = "normal",  frame = 17289, emit = true },
    { type = "normal",  frame = 17484, emit = true },
    -- map_exit_1E (PPU $2000 indirection handled internally by the game)
    { type = "normal",  frame = 18793, emit = true },
    -- rocket_entry: silent (4-byte mutation only used; 128-byte step's output is overwritten)
    { type = "silent",  frame = 18829, emit = false },
    -- map_exit_1B (also has PPU $2000 indirection internally)
    { type = "normal",  frame = 19367, emit = true },
    { type = "normal",  frame = 20555, emit = true },
    { type = "normal",  frame = 21804, emit = true },
    -- car_entry (split): 4-byte at f22734, 128-byte at f23041
    { type = "delayed", frame = 22733, frame2 = 23040, emit = true },
    { type = "normal",  frame = 25027, emit = true },
    { type = "normal",  frame = 31961, emit = true },
    { type = "normal",  frame = 33681, emit = true },
    { type = "normal",  frame = 34249, emit = true },
    { type = "normal",  frame = 34841, emit = true },
    -- machine_part (split): 4-byte at f37680, 128-byte at f38668 (= map_exit_22_again)
    { type = "delayed", frame = 37679, frame2 = 38667, emit = true },
    { type = "normal",  frame = 41860, emit = true },
    { type = "normal",  frame = 43222, emit = true },
    { type = "normal",  frame = 49666, emit = true },
    { type = "normal",  frame = 50268, emit = true },
    { type = "normal",  frame = 54196, emit = true },
    { type = "normal",  frame = 54666, emit = true },
    { type = "normal",  frame = 58654, emit = true },
    { type = "normal",  frame = 61565, emit = true },
    { type = "normal",  frame = 63885, emit = true },
}

-- ============================================================================
-- Pre-load savestate objects (each can be reused across records)
-- ============================================================================
local function preload_savestate(frame)
    local path = lib.savestates[frame].file
    local f = io.open(path, "rb")
    if not f then
        error("savestate not found: " .. path)
    end
    f:close()
    return savestate.create(path)
end

local SS_EXPANSION = preload_savestate(2549)
for i, evt in ipairs(CHAIN) do
    evt.ss1_obj = preload_savestate(evt.frame)
    if evt.frame2 then
        evt.ss2_obj = preload_savestate(evt.frame2)
    end
end

print(string.format("[run_all_maps] %d savestate objects pre-loaded",
    1 + #CHAIN + (function() local n = 0; for _, e in ipairs(CHAIN) do if e.frame2 then n = n + 1 end end; return n end)()))

-- ============================================================================
-- Stage A hook: $B112 expansion
-- ============================================================================
local pending_key = nil
memory.registerexecute(EXPANSION_PC, function()
    if not pending_key then return end
    lib.write_range(BUFFER_BASE, pending_key)
    pending_key = nil
end)

-- ============================================================================
-- Stage B hook: $82C3 with X=0 (round-0 buf128 inject)
-- ============================================================================
local pending_buf128 = nil
memory.registerexecute(STAGE10_FIRST_PC, function()
    if not pending_buf128 then return end
    if memory.getregister("x") ~= 0 then return end
    lib.write_range(BUFFER_BASE, pending_buf128)
    pending_buf128 = nil
end)

-- ============================================================================
-- Stage 10 alg capture: fires on every $EE6B dispatch; we count only those
-- in the 16-round chain's range $1F..$26 to know which algs ran.
-- ============================================================================
local current_algs = {}
local function reset_algs()
    for i = #current_algs, 1, -1 do current_algs[i] = nil end
end
memory.registerexecute(DISPATCHER_PC, function()
    local a = memory.getregister("a")
    if a >= 0x1F and a <= 0x26 then
        current_algs[#current_algs + 1] = a - 0x1F
    end
end)

-- ============================================================================
-- Post-route-$1E capture hook: when CPU reaches $82EC (the first instruction
-- after route $1E has finished writing $0200..$027F back to PPU), grab a
-- snapshot of state4 + buf128. This is the surgical "the chain is done" signal.
-- ============================================================================
local capture_armed = false
local captured_state4 = nil
local captured_buf128 = nil

memory.registerexecute(STAGE10_DONE_PC, function()
    if not capture_armed then return end
    captured_state4 = lib.read_range(STATE4_BASE, 4)
    captured_buf128 = lib.read_range(BUFFER_BASE, BUFFER_LEN)
    capture_armed = false
end)

-- ============================================================================
-- Frame-advance helpers
-- ============================================================================
local function advance_to(target_frame)
    while emu.framecount() < target_frame do emu.frameadvance() end
end

-- Frame-advance until the post-route-$1E hook has snapshotted state, with
-- a safety cap. Returns (state4, buf128) or nil if the hook didn't fire.
local function advance_until_captured(expected_frame)
    local cap = expected_frame + MAX_LEAD
    while capture_armed and emu.framecount() < cap do
        emu.frameadvance()
    end
    if capture_armed then
        capture_armed = false   -- disarm; will fail with nil return
        return nil, nil
    end
    return captured_state4, captured_buf128
end

-- ============================================================================
-- Stage A: expansion ($B112) — produces (state4_initial, buf128_initial)
-- ============================================================================
local function run_expansion(key8)
    savestate.load(SS_EXPANSION)
    pending_key = key8
    -- Frame-advance through f2550 so $B112 fires; expansion completes
    -- within one frame.
    advance_to(emu.framecount() + 3)
    if pending_key then
        return nil, nil, "$B112 did not fire"
    end
    -- Initial state4 = key[0..3] (Stage 6 also seeds RNG from key[0..1] but
    -- restores it; the carried-forward state4 going into event 2 is key[0..3]).
    local state4 = string.sub(key8, 1, 4)
    local buf128 = lib.read_range(BUFFER_BASE, BUFFER_LEN)
    return state4, buf128
end

-- ============================================================================
-- Stage B: per-event runner
-- ============================================================================
local function inject_state4(state)
    lib.write_range(STATE4_BASE, state)
end
local function read_state4()
    return lib.read_range(STATE4_BASE, 4)
end
local function inject_buffer128(buf)
    lib.write_range(BUFFER_BASE, buf)
end
local function read_buffer128()
    return lib.read_range(BUFFER_BASE, BUFFER_LEN)
end

local function run_event(evt, state4, buf128)
    if evt.type == "normal" then
        savestate.load(evt.ss1_obj)
        inject_state4(state4)
        pending_buf128 = buf128       -- $82C3 hook will inject just before round 0
        reset_algs()
        captured_state4, captured_buf128 = nil, nil
        capture_armed = true          -- $82EC hook will snapshot when route $1E completes
        local s4, b128 = advance_until_captured(evt.frame)
        if not s4 then
            return nil, nil, "post-route-$1E hook did not fire"
        end
        return s4, b128

    elseif evt.type == "silent" then
        -- 4-byte mutation only; buf128 carries through unchanged. No Stage 10
        -- runs, so $82EC won't fire — fall back to a short time-based wait.
        savestate.load(evt.ss1_obj)
        inject_state4(state4)
        inject_buffer128(buf128)
        reset_algs()
        advance_to(evt.frame + POST_LEAD)
        return read_state4(), buf128

    elseif evt.type == "delayed" then
        -- Sub-step A: 4-byte mutation only — no Stage 10 fires here, so use
        -- the time-based wait.
        savestate.load(evt.ss1_obj)
        inject_state4(state4)
        inject_buffer128(buf128)
        advance_to(evt.frame + POST_LEAD)
        local new_state4 = read_state4()

        -- Sub-step B: delayed 128-byte step — Stage 10 runs, $82EC fires.
        savestate.load(evt.ss2_obj)
        inject_state4(new_state4)
        pending_buf128 = buf128
        reset_algs()
        captured_state4, captured_buf128 = nil, nil
        capture_armed = true
        local s4, b128 = advance_until_captured(evt.frame2)
        if not s4 then
            return nil, nil, "post-route-$1E hook did not fire (delayed sub-step B)"
        end
        return s4, b128

    else
        error("unknown event type: " .. tostring(evt.type))
    end
end

-- ============================================================================
-- Optional debug output
-- ============================================================================
local debug_f = nil
if DEBUG_FILE then
    debug_f = io.open(DEBUG_FILE, "w")
    if debug_f then
        print(string.format("[run_all_maps] writing per-step debug to %s", DEBUG_FILE))
    end
end

local function debug_dump(record_idx, step_idx, label, state4, buf128)
    if not debug_f then return end
    local s4 = {}
    for i = 1, 4 do s4[i] = string.format("%02X", string.byte(state4, i)) end
    local b1 = {}
    for i = 1, 16 do b1[i] = string.format("%02X", string.byte(buf128, i)) end
    local algs_str = ""
    if #current_algs > 0 then
        local at = {}
        for i, a in ipairs(current_algs) do at[i] = tostring(a) end
        algs_str = "  algs: " .. table.concat(at, " ")
    end
    debug_f:write(string.format(
        "rec %2d  step %2d  %-30s state4=%s  buf128[0..15]=%s%s\n",
        record_idx, step_idx, label,
        table.concat(s4, " "), table.concat(b1, " "), algs_str))
    -- Full buffer (4 rows × 32 bytes)
    debug_f:write("    buf128 full:\n")
    for row = 0, 3 do
        local rb = {}
        for i = 1, 32 do
            rb[i] = string.format("%02X", string.byte(buf128, row * 32 + i))
        end
        debug_f:write("      " .. table.concat(rb, " ") .. "\n")
    end
end

-- ============================================================================
-- Run a full chain on one 8-byte key
-- ============================================================================
local function run_chain(key8, record_idx)
    -- Stage A: expansion
    local state4, buf128, err = run_expansion(key8)
    if not state4 then return nil, err end
    debug_dump(record_idx, 0, "post-expansion (initial)", state4, buf128)

    -- Stage B: 28 events
    for i, evt in ipairs(CHAIN) do
        local new_s4, new_b128, perr = run_event(evt, state4, buf128)
        if not new_s4 then
            return nil, string.format("event %d (%s) failed: %s",
                i, lib.savestates[evt.frame].name, perr or "(no error)")
        end
        state4, buf128 = new_s4, new_b128
        local ss = lib.savestates[evt.frame]
        local label = string.format("after %s (%s)", ss.name, evt.type)
        debug_dump(record_idx, i, label, state4, buf128)
    end

    if debug_f then debug_f:flush() end
    return buf128, nil
end

-- ============================================================================
-- Main loop
-- ============================================================================
local reader, err = lib.open_inputs(INPUTS_FILE, lib.TEST_TYPE_WC_ALL_MAPS,
    SUBTYPE, RECORD_SIZE_IN)
if not reader then
    print("ERROR: " .. err)
    return
end

local total = reader.record_count
if MAX_TESTS and MAX_TESTS < total then total = MAX_TESTS end

local out_f = lib.open_outputs(OUTPUTS_FILE, lib.TEST_TYPE_WC_ALL_MAPS,
    SUBTYPE, RECORD_SIZE_OUT, total, reader.shared_count)
if not out_f then
    print("ERROR: failed to open outputs")
    lib.close_inputs(reader)
    return
end

print(string.format("# === all_maps (expansion + 28 events, 27 emit) ==="))
print(string.format("#   inputs:  %s  (records=%d)", INPUTS_FILE, reader.record_count))
print(string.format("#   outputs: %s", OUTPUTS_FILE))
if MAX_TESTS then print(string.format("#   MAX_TESTS=%d", MAX_TESTS)) end

local start_time = os.time()
for test_idx = 1, total do
    local key = lib.read_record(reader)
    if not key or #key < RECORD_SIZE_IN then
        print(string.format("#   short read at %d/%d", test_idx, total))
        break
    end
    local final_buf, perr = run_chain(key, test_idx - 1)
    if not final_buf then
        print(string.format("#   test %d/%d FAILED: %s", test_idx, total, perr))
        break
    end
    -- Output row: [8] key + [27 × (state4 + buf128)] + [final state4 + final buf128].
    -- The intermediate (state4, buf128) snapshots are accumulated by run_chain
    -- in the debug file when DEBUG_FILE is set; for the binary output we just
    -- emit the final buffer per the historical format.
    -- TODO: if your C++ tester needs intermediate states in the binary too,
    -- accumulate them in run_chain and write here.
    out_f:write(key)
    out_f:write(final_buf)
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
if debug_f then debug_f:close() end
local elapsed = os.time() - start_time
local rate    = elapsed > 0 and (total / elapsed) or 0
print(string.format("# all done. %d/%d records in %ds (%.2f rec/s)",
    total, total, elapsed, rate))
