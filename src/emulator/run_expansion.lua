-- run_expansion.lua
--
-- Runs the 8-byte → 128-byte input expansion routine ($B112) for each
-- key in test_inputs/expansion.inputs.bin and writes the 128-byte result
-- per record to test_outputs/expansion.bin.
--
-- Approach: load the F02550_expansion savestate (1 frame before $B112
-- naturally fires). Hook on $B112; when fired, overwrite $0200..$0207
-- with our 8-byte input key. Frame-advance a couple frames so the
-- expansion routine completes (it writes $0208..$027F via RNG). Read
-- the full 128 bytes back from $0200..$027F as the result.
--
-- No trampoline, no bank switching, no sentinel hooks, no PPU dance.
-- The natural game flow runs the routine in its own context; we just
-- substitute our key at the moment it'd be read.

local lib = dofile((function()
    local src = debug.getinfo(1, "S").source
    if src:sub(1, 1) == "@" then src = src:sub(2) end
    return (src:match("(.*[/\\])") or "") .. "lib_test.lua"
end)())

-- ============================================================================
-- Configuration
-- ============================================================================
local INPUTS_FILE       = lib.test_inputs_dir  .. "expansion.inputs.bin"
local OUTPUTS_FILE      = lib.test_outputs_dir .. "expansion.bin"
local PROGRESS_INTERVAL = 5000
local MAX_TESTS         = nil    -- nil = process all records

local STAGE_PC          = 0xB112  -- expansion entry: reads $0200..$0207, writes $0200..$027F
local KEY_BASE          = 0x0200  -- 8-byte input
local BUF_BASE          = 0x0200  -- 128-byte result lives here
local BUF_LEN           = 128
local SETTLE_FRAMES     = 2       -- expansion completes within 1 frame; 2 is safe

-- TMTV record sizes
local RECORD_SIZE_IN    = 8                    -- 8-byte key
local RECORD_SIZE_OUT   = 8 + BUF_LEN          -- echo key + 128-byte buffer
local SUBTYPE           = 0                    -- expansion has no chain depth; subtype is N/A

-- ============================================================================
-- Hook: when $B112 is about to execute, inject our 8-byte key into $0200..$0207
-- ============================================================================
local pending_key = nil

memory.registerexecute(STAGE_PC, function()
    if not pending_key then return end
    lib.write_range(KEY_BASE, pending_key)
    pending_key = nil
end)

-- ============================================================================
-- Anchor savestate setup
-- ============================================================================
local anchor_path = lib.savestates[2549].file
local anchor_obj
do
    local f = io.open(anchor_path, "rb")
    if not f then
        print("ERROR: anchor savestate not found at " .. anchor_path)
        return
    end
    f:close()
    anchor_obj = savestate.create(anchor_path)
end

-- ============================================================================
-- Per-record runner
-- ============================================================================
local function run_one(key8)
    savestate.load(anchor_obj)
    pending_key = key8
    -- Frame-advance: $B112 fires within the first frame after the anchor;
    -- our hook injects the key, expansion runs to completion, and a few
    -- extra frames let the buffer settle without anything overwriting it.
    for _ = 1, SETTLE_FRAMES do
        emu.frameadvance()
    end
    if pending_key then
        return nil, "$B112 did not fire"
    end
    return lib.read_range(BUF_BASE, BUF_LEN)
end

-- ============================================================================
-- Main loop
-- ============================================================================
local reader, err = lib.open_inputs(INPUTS_FILE, lib.TEST_TYPE_EXPANSION, SUBTYPE, RECORD_SIZE_IN)
if not reader then
    print("ERROR: " .. err)
    return
end

local total = reader.record_count
if MAX_TESTS and MAX_TESTS < total then total = MAX_TESTS end

local out_f = lib.open_outputs(OUTPUTS_FILE, lib.TEST_TYPE_EXPANSION, SUBTYPE,
    RECORD_SIZE_OUT, total, reader.shared_count)
if not out_f then
    print("ERROR: failed to open outputs")
    lib.close_inputs(reader)
    return
end

print(string.format("# === expansion ($B112: 8 → 128 bytes) ==="))
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
    local buf, perr = run_one(key)
    if not buf then
        print(string.format("#   test %d/%d FAILED: %s", test_idx, total, perr))
        break
    end
    out_f:write(key)
    out_f:write(buf)
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
