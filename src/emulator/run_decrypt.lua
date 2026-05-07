-- run_decrypt.lua
--
-- Tests the World-5 decryption routine ($7:FC65 onward — the XOR loop at
-- $FC7A..$FC86 that XORs a 128-byte keystream against an encrypted ROM
-- blob into $06A5..$06A5+length-1, then dispatches the checksum). Two
-- entries (different blobs/lengths):
--   decrypt0.inputs.bin  → entry 0  (length 114)
--   decrypt1.inputs.bin  → entry 1  (length 83)
--
-- TMTV format:
--   in:  [128] keystream
--   out: [128] keystream + [length] decrypted_bytes
--   test_type = 0x08
--
-- Approach (natural flow + override):
--   • Anchor on F64430_decrypt_checksum (10 frames before the World-5
--     decryption pipeline naturally enters $FC65).
--   • Hook on $FC65 (decryption routine entry, where it reads $46 to
--     index parameter tables). When fired:
--       - override $0200..$027F with our keystream,
--       - override $46 with our test entry index.
--     The natural game flow does the bank-switch and PPU read up to this
--     point; we just substitute the inputs.
--   • Hook on $FC88 (LDY $46 — the instruction immediately after the XOR
--     loop's BNE exits). At this point the XOR has finished and
--     $06A5..$06A5+length-1 holds the decrypted bytes. Length is in $42
--     (the routine just stored it from $FCAB,X). Capture both.

local lib = dofile((function()
    local src = debug.getinfo(1, "S").source
    if src:sub(1, 1) == "@" then src = src:sub(2) end
    return (src:match("(.*[/\\])") or "") .. "lib_test.lua"
end)())

-- ============================================================================
-- Configuration
-- ============================================================================
local PROGRESS_INTERVAL = 200
local MAX_TESTS         = nil
local MAX_LEAD_FRAMES   = 30
local ENTRIES           = nil    -- nil = both 0 and 1; or {0} / {1} for subset

-- ============================================================================
-- Constants
-- ============================================================================
local DECRYPT_ENTRY_PC = 0xFC65    -- LDX $46 — start of routine after bank-swap & PPU-read
local POST_XOR_PC      = 0xFC88    -- LDY $46 — first instr after XOR loop

local KEYSTREAM_BASE   = 0x0200    -- 128-byte keystream
local KEYSTREAM_LEN    = 128
local DECRYPT_BASE     = 0x06A5    -- decrypted output
local LENGTH_ADDR      = 0x0042    -- routine writes length here
local ENTRY_INDEX      = 0x0046    -- $46 = entry index

local DECRYPT_LEN = { [0] = 114, [1] = 83 }

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
-- Per-record state
-- ============================================================================
local pending_keystream = nil
local pending_entry     = nil
local capture_armed     = false
local captured_length   = nil
local captured_bytes    = nil

-- ============================================================================
-- Hooks
-- ============================================================================

-- $FC65: about to LDX $46 — first instr of the per-entry decryption setup.
-- Override keystream + entry index. Fires once per record.
memory.registerexecute(DECRYPT_ENTRY_PC, function()
    if not pending_keystream then return end
    lib.write_range(KEYSTREAM_BASE, pending_keystream)
    memory.writebyte(ENTRY_INDEX, pending_entry)
    pending_keystream = nil
end)

-- $FC88: LDY $46 — first instr after the XOR loop exits. $42 has the
-- length the routine just used. $06A5..$06A5+length-1 has the result.
memory.registerexecute(POST_XOR_PC, function()
    if not capture_armed then return end
    captured_length = memory.readbyte(LENGTH_ADDR)
    captured_bytes  = lib.read_range(DECRYPT_BASE, captured_length)
    capture_armed   = false
end)

-- ============================================================================
-- Per-record runner
-- ============================================================================
local function run_one(keystream, entry_idx)
    savestate.load(anchor_obj)

    pending_keystream = keystream
    pending_entry     = entry_idx
    captured_length   = nil
    captured_bytes    = nil
    capture_armed     = true

    local cap_frame = emu.framecount() + MAX_LEAD_FRAMES
    while capture_armed and emu.framecount() < cap_frame do
        emu.frameadvance()
    end
    if capture_armed then
        capture_armed = false
        return nil, "$FC88 capture hook did not fire"
    end
    return captured_bytes
end

-- ============================================================================
-- Process one entry's inputs file
-- ============================================================================
local function process_entry(entry_idx)
    local decrypt_len = DECRYPT_LEN[entry_idx]
    local in_path     = lib.test_inputs_dir  .. string.format("decrypt%d.inputs.bin", entry_idx)
    local out_path    = lib.test_outputs_dir .. string.format("decrypt%d.bin",        entry_idx)

    local reader, err = lib.open_inputs(in_path, 0x08, entry_idx, KEYSTREAM_LEN)
    if not reader then
        print(string.format("[decrypt %d] SKIP: %s", entry_idx, err))
        return
    end

    local total = reader.record_count
    if MAX_TESTS and MAX_TESTS < total then total = MAX_TESTS end

    local out_f = lib.open_outputs(out_path, 0x08, entry_idx,
        KEYSTREAM_LEN + decrypt_len, total, reader.shared_count)
    if not out_f then
        print(string.format("[decrypt %d] ERROR opening output", entry_idx))
        lib.close_inputs(reader)
        return
    end

    print(string.format("# === decrypt entry %d (length=%d) ===", entry_idx, decrypt_len))
    print(string.format("#   inputs:  %s  (records=%d)", in_path, reader.record_count))
    print(string.format("#   outputs: %s", out_path))
    if MAX_TESTS then print(string.format("#   MAX_TESTS=%d", MAX_TESTS)) end

    local start_time = os.time()
    for test_idx = 1, total do
        local keystream = lib.read_record(reader)
        if not keystream or #keystream < KEYSTREAM_LEN then
            print(string.format("#   short read at %d/%d", test_idx, total))
            break
        end

        local decrypted, perr = run_one(keystream, entry_idx)
        if not decrypted then
            print(string.format("#   test %d/%d FAILED: %s", test_idx, total, perr))
            break
        end
        if #decrypted ~= decrypt_len then
            print(string.format("#   test %d/%d length mismatch: got %d, expected %d",
                test_idx, total, #decrypted, decrypt_len))
            break
        end

        out_f:write(keystream)
        out_f:write(decrypted)

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
    print(string.format("# decrypt %d done. %d/%d records in %ds (%.2f rec/s)",
        entry_idx, total, total, elapsed, rate))
end

-- ============================================================================
-- Main
-- ============================================================================
local entries = ENTRIES or {0, 1}
for _, e in ipairs(entries) do
    if DECRYPT_LEN[e] then
        process_entry(e)
    else
        print(string.format("[decrypt] skipping invalid entry %d", e))
    end
end

print("# all decrypt entries processed")
