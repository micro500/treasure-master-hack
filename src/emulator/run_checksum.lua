-- run_checksum.lua
--
-- Tests the password-validation checksum routine ($7:FD18, dispatched via
-- $EE6B with A=$1C). Two TMTV files (one per entry index in the parameter
-- tables at $FD50/57/5E/65):
--   checksum0.inputs.bin  → entry 0 (X=2, sum_length=112)
--   checksum1.inputs.bin  → entry 1 (X=3, sum_length=81)
--
-- Per record:
--   in:  [sum_length+2] buffer (the last 2 bytes are the stored checksum
--                                that the routine compares against)
--   out: input buffer + [2] computed_sum + [2] stored_sum + [1] match_flag
--
-- Approach (Lua-level trampoline + hook capture):
--   1. Anchor on F64430_decrypt_checksum (~10 frames before the World 5
--      decryption pipeline naturally dispatches checksum).
--   2. Inject buffer at $06A5..$06A5+sum_length+1.
--   3. Set A = $1C (checksum routine ID), X = entry's checksum-table index.
--      Push sentinel-1 onto the stack so the routine's eventual RTS lands
--      somewhere harmless.
--   4. Set PC = $EE6B (post-inline-fetch dispatcher entry).
--   5. Frame-advance. Hook on $EE8D fires once the routine has completed
--      and is about to RTS back through the dispatcher; capture the
--      16-bit accumulator at $42/$43 plus the stored bytes from the
--      buffer, and set the match flag.

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
local ENTRIES           = nil    -- nil = both 0 and 1; or {0} / {1} for subset

-- ============================================================================
-- Constants
-- ============================================================================
local DISPATCHER_PC = 0xEE6B
local SENTINEL      = 0x00C0
local POST_ROUTINE  = 0xEE8D    -- snapshot point just after dispatched routine RTS

local BUFFER_BASE   = 0x06A5
local CKSUM_LO      = 0x0042    -- accumulator lo
local CKSUM_HI      = 0x0043    -- accumulator hi
local CHECKSUM_RID  = 0x1C

-- entry → (checksum-table X, sum_length, total record size)
local ENTRY_X     = { [0] = 2,   [1] = 3  }
local SUM_LEN     = { [0] = 112, [1] = 81 }
local RECORD_SIZE = { [0] = 114, [1] = 83 }

-- ============================================================================
-- Anchor savestate
-- ============================================================================
local ANCHOR_FRAME = 64430
local anchor_obj
do
    local entry = lib.savestates[ANCHOR_FRAME]
    if not entry then
        print("ERROR: no savestate registered at frame " .. ANCHOR_FRAME)
        return
    end
    local f = io.open(entry.file, "rb")
    if not f then
        print("ERROR: anchor savestate file not found: " .. entry.file)
        return
    end
    f:close()
    anchor_obj = savestate.create(entry.file)
end

-- ============================================================================
-- Capture hook: post-routine RTS
-- ============================================================================
local capture_armed   = false
local captured_lo     = nil
local captured_hi     = nil

memory.registerexecute(POST_ROUTINE, function()
    if not capture_armed then return end
    captured_lo  = memory.readbyte(CKSUM_LO)
    captured_hi  = memory.readbyte(CKSUM_HI)
    capture_armed = false
end)

-- ============================================================================
-- Per-record runner
-- ============================================================================
local function run_one(buffer_full, entry_idx, sum_length)
    savestate.load(anchor_obj)

    -- Inject the entire input buffer (sum_length + 2 stored bytes) at $06A5.
    lib.write_range(BUFFER_BASE, buffer_full)

    -- Trampoline setup: push sentinel-1 onto stack so the routine's RTS
    -- lands at $00C0 (harmless), set A = checksum routine ID, X = entry's
    -- parameter-table index, and jump CPU to the dispatcher.
    local sp     = memory.getregister("s")
    local target = SENTINEL - 1   -- pushed value; RTS pops + 1 → SENTINEL
    memory.writebyte(0x0100 + sp,         math.floor(target / 256))    -- hi
    memory.writebyte(0x0100 + ((sp - 1) % 256), target % 256)          -- lo
    memory.setregister("s", (sp - 2) % 256)
    memory.setregister("a", CHECKSUM_RID)
    memory.setregister("x", ENTRY_X[entry_idx])
    memory.setregister("pc", DISPATCHER_PC)

    captured_lo   = nil
    captured_hi   = nil
    capture_armed = true

    local cap_frame = emu.framecount() + MAX_LEAD_FRAMES
    while capture_armed and emu.framecount() < cap_frame do
        emu.frameadvance()
    end
    if capture_armed then
        capture_armed = false
        return nil, "$EE8D capture hook did not fire"
    end

    local stored_lo = string.byte(buffer_full, sum_length + 1)
    local stored_hi = string.byte(buffer_full, sum_length + 2)
    local match     = (captured_lo == stored_lo and captured_hi == stored_hi) and 1 or 0
    return captured_lo, captured_hi, stored_lo, stored_hi, match
end

-- ============================================================================
-- Process one entry's inputs file
-- ============================================================================
local function process_entry(entry_idx)
    local sum_length  = SUM_LEN[entry_idx]
    local record_size = RECORD_SIZE[entry_idx]

    local in_path  = lib.test_inputs_dir  .. string.format("checksum%d.inputs.bin", entry_idx)
    local out_path = lib.test_outputs_dir .. string.format("checksum%d.bin",        entry_idx)

    local reader, err = lib.open_inputs(in_path, 0x09, entry_idx, record_size)
    if not reader then
        print(string.format("[checksum %d] SKIP: %s", entry_idx, err))
        return
    end

    local total = reader.record_count
    if MAX_TESTS and MAX_TESTS < total then total = MAX_TESTS end

    local out_f = lib.open_outputs(out_path, 0x09, entry_idx, record_size + 5, total, reader.shared_count)
    if not out_f then
        print(string.format("[checksum %d] ERROR opening output", entry_idx))
        lib.close_inputs(reader)
        return
    end

    print(string.format("# === checksum entry %d (X=%d, sum_length=%d) ===",
        entry_idx, ENTRY_X[entry_idx], sum_length))
    print(string.format("#   inputs:  %s  (records=%d)", in_path, reader.record_count))
    print(string.format("#   outputs: %s", out_path))
    if MAX_TESTS then print(string.format("#   MAX_TESTS=%d", MAX_TESTS)) end

    local start_time = os.time()
    for test_idx = 1, total do
        local rec = lib.read_record(reader)
        if not rec or #rec < record_size then
            print(string.format("#   short read at %d/%d", test_idx, total))
            break
        end

        local clo, chi, slo, shi, match = run_one(rec, entry_idx, sum_length)
        if not clo then
            print(string.format("#   test %d/%d FAILED: %s", test_idx, total, chi))
            break
        end

        out_f:write(rec)
        out_f:write(string.char(clo, chi))     -- computed
        out_f:write(string.char(slo, shi))     -- stored
        out_f:write(string.char(match))        -- 1=match, 0=mismatch

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
    print(string.format("# checksum %d done. %d/%d records in %ds (%.2f rec/s)",
        entry_idx, total, total, elapsed, rate))
end

-- ============================================================================
-- Main
-- ============================================================================
local entries = ENTRIES or {0, 1}
for _, e in ipairs(entries) do
    if ENTRY_X[e] then
        process_entry(e)
    else
        print(string.format("[checksum] skipping invalid entry %d", e))
    end
end

print("# all checksum entries processed")
