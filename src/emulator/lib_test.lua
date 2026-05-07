-- lib_test.lua
--
-- Shared utilities for the Treasure Master test harnesses:
--   • Savestate registry — maps frame number → filename + description
--   • TMTV file I/O      — read inputs, write outputs (32-byte header + records)
--   • Memory helpers     — read_range / write_range for CPU RAM
--
-- Each harness loads this via:
--   local lib = dofile(script_dir() .. "lib_test.lua")
--
-- and accesses, e.g.:
--   savestate.load(savestate.create(lib.savestates[2550].file))
--   local reader = lib.open_inputs(path, lib.TEST_TYPE_EXPANSION, 27, 8)
--   lib.write_range(0x0200, key_bytes)

local M = {}

-- ============================================================================
-- Project root (directory containing this lib file)
-- ============================================================================
local function _script_dir()
    local src = debug.getinfo(1, "S").source
    if src:sub(1, 1) == "@" then src = src:sub(2) end
    return src:match("(.*[/\\])") or ""
end

M.dir              = _script_dir()
M.savestates_dir   = M.dir .. "savestates/"
M.test_inputs_dir  = M.dir .. "test_inputs/"
M.test_outputs_dir = M.dir .. "test_outputs/"

-- ============================================================================
-- Savestate registry
--
-- Each savestate captures the emulator state ~1 frame before a specific
-- moment in the password-screen processing. Listed in chronological order
-- of when their corresponding event fires during the TAS playback.
--
-- Filename pattern: savestates/F<5-digit-frame>_<descriptive-name>.fcs
-- ============================================================================
local function _ss(frame, name, purpose)
    local fname = string.format("F%05d_%s.fcs", frame, name)
    return {
        frame   = frame,
        name    = name,
        file    = M.savestates_dir .. fname,
        purpose = purpose,
    }
end

-- Registry keyed by save_frame (the actual frame the .fcs file captures).
-- The associated event/dispatch normally fires 1 frame later (or further
-- ahead for anchors that need lead time).
M.savestates = {
    [ 2549] = _ss( 2549, "expansion",
        "1 frame before $B112 expansion: 8-byte input at $0200..$0207 → 128-byte buffer at $0200..$027F"),
    [ 3161] = _ss( 3161, "map_exit_00",        "1f before exit of map $00 (first key-mixing event)"),
    [ 8941] = _ss( 8941, "map_exit_02",        "1f before exit of map $02"),
    [ 9352] = _ss( 9352, "map_exit_05",        "1f before exit of map $05"),
    [ 9549] = _ss( 9549, "map_exit_04",        "1f before exit of map $04"),
    [ 9744] = _ss( 9744, "map_exit_03",        "1f before exit of map $03"),
    [17289] = _ss(17289, "map_exit_1D",        "1f before exit of map $1D"),
    [17484] = _ss(17484, "map_exit_1C",        "1f before exit of map $1C"),
    [18793] = _ss(18793, "map_exit_1E",
        "1f before exit of map $1E. Input buffer comes from PPU $2000..$207F via _LFCB6 archive."),
    [18829] = _ss(18829, "rocket_entry",
        "1f before rocket_entry (silent — sprite-driven $2D dispatch; 128-byte output overwritten before anything reads it)"),
    [19367] = _ss(19367, "map_exit_1B",        "1f before exit of map $1B (same PPU $2000 indirection as map_exit_1E)"),
    [20555] = _ss(20555, "map_exit_07",        "1f before exit of map $07"),
    [21804] = _ss(21804, "map_exit_08",        "1f before exit of map $08"),
    [22733] = _ss(22733, "car_entry",
        "1f before car_entry: 4-byte mutation half of a split event (128-byte mutation runs at car_scene_entry)"),
    [23040] = _ss(23040, "car_scene_entry",
        "1f before car_scene_entry: 128-byte half of the car_entry split event"),
    [25027] = _ss(25027, "map_exit_09",        "1f before exit of map $09"),
    [31961] = _ss(31961, "map_exit_0C",        "1f before exit of map $0C"),
    [33681] = _ss(33681, "map_exit_20",        "1f before exit of map $20"),
    [34249] = _ss(34249, "map_exit_21",        "1f before exit of map $21"),
    [34841] = _ss(34841, "map_exit_22",        "1f before exit of map $22 (first time)"),
    [37679] = _ss(37679, "machine_part",
        "1f before machine_part: 4-byte mutation half of a split event (128-byte mutation runs at map_exit_22_again)"),
    [38667] = _ss(38667, "map_exit_22_again",
        "1f before exit of map $22 (second time). 128-byte half of the machine_part split event."),
    [41860] = _ss(41860, "map_exit_23",        "1f before exit of map $23"),
    [43222] = _ss(43222, "map_exit_24",        "1f before exit of map $24"),
    [49666] = _ss(49666, "map_exit_25",        "1f before exit of map $25"),
    [50268] = _ss(50268, "map_exit_26",        "1f before exit of map $26"),
    [54196] = _ss(54196, "map_exit_0E",        "1f before exit of map $0E"),
    [54666] = _ss(54666, "map_exit_0F",        "1f before exit of map $0F"),
    [58654] = _ss(58654, "map_exit_10",        "1f before exit of map $10"),
    [61565] = _ss(61565, "map_exit_12",        "1f before exit of map $12"),
    [63885] = _ss(63885, "map_exit_11",        "1f before exit of map $11 (last event in the chain)"),
    [64430] = _ss(64430, "decrypt_checksum",
        "10 frames before the World 5 decryption pipeline dispatches the checksum routine ($EE6B with A=$1C)"),
}

-- Helper to get a savestate's full file path by frame.
function M.savestate_path(frame)
    local entry = M.savestates[frame]
    if not entry then
        error(string.format("no savestate registered at frame %d", frame))
    end
    return entry.file
end

-- ============================================================================
-- TMTV format constants
-- ============================================================================
M.TMTV_MAGIC               = "TMTV"
M.TMTV_VERSION             = 1
M.TMTV_HEADER_SIZE         = 32
M.RECORD_KIND_FULL         = 0x00
M.RECORD_KIND_INPUTS_ONLY  = 0x01

-- Test type IDs, ordered roughly by how they happen in the game's
-- password-screen flow:
--   key schedule (Stage 9)  → expansion ($B112) → working code (Stage 10)
--   → World 5 decryption → checksum verification.
M.TEST_TYPE_KS_DISPATCH    = 0x01    -- run_key_schedule_dispatch
M.TEST_TYPE_KS_ALG         = 0x02    -- run_key_schedule_alg
M.TEST_TYPE_KS_ALL_MAPS    = 0x03    -- run_key_schedule_all_maps
M.TEST_TYPE_EXPANSION      = 0x04    -- run_expansion
M.TEST_TYPE_WC_ALG         = 0x05    -- run_wc_alg
M.TEST_TYPE_WC_ALG_MULTI   = 0x06    -- run_wc_alg_multi (N-alg chain)
M.TEST_TYPE_WC_ALL_MAPS    = 0x07    -- run_all_maps (full Stage 8/9/10 chain)
M.TEST_TYPE_DECRYPT        = 0x08    -- run_decrypt
M.TEST_TYPE_CHECKSUM       = 0x09    -- run_checksum

-- ============================================================================
-- Binary helpers
-- ============================================================================
function M.u16_le_bytes(n)
    return string.char(n % 256, math.floor(n / 256) % 256)
end

function M.u32_le_bytes(n)
    return string.char(
        n        % 256,
        math.floor(n /        256) % 256,
        math.floor(n /      65536) % 256,
        math.floor(n /   16777216) % 256)
end

function M.read_u16_le(s, offset)
    return string.byte(s, offset + 1) * 256 + string.byte(s, offset)
end

function M.read_u32_le(s, offset)
    return string.byte(s, offset + 3) * 16777216
         + string.byte(s, offset + 2) * 65536
         + string.byte(s, offset + 1) * 256
         + string.byte(s, offset)
end

-- ============================================================================
-- Inputs file: open + read records
-- ============================================================================
-- Returns a reader handle (or nil + error message on failure).
-- expected_* args are optional; if provided they're validated against the
-- header and any mismatch returns an error.
function M.open_inputs(path, expected_test_type, expected_subtype, expected_record_size)
    local f = io.open(path, "rb")
    if not f then
        return nil, "inputs file not found: " .. path
    end
    local hdr = f:read(M.TMTV_HEADER_SIZE)
    if not hdr or #hdr < M.TMTV_HEADER_SIZE then
        f:close()
        return nil, "inputs header too short"
    end
    local magic        = hdr:sub(1, 4)
    local version      = M.read_u16_le(hdr, 5)
    local test_type    = string.byte(hdr, 7)
    local subtype      = string.byte(hdr, 8)
    local record_size  = M.read_u16_le(hdr, 9)
    local record_count = M.read_u32_le(hdr, 11)
    local shared_count = M.read_u32_le(hdr, 15)
    local record_kind  = string.byte(hdr, 19)

    if magic ~= M.TMTV_MAGIC then f:close(); return nil, "bad magic" end
    if version ~= M.TMTV_VERSION then f:close(); return nil, "bad version" end
    if record_kind ~= M.RECORD_KIND_INPUTS_ONLY then
        f:close(); return nil, "record_kind != inputs-only"
    end
    if expected_test_type and test_type ~= expected_test_type then
        f:close()
        return nil, string.format("test_type=0x%02X, expected 0x%02X",
            test_type, expected_test_type)
    end
    if expected_subtype and subtype ~= expected_subtype then
        f:close()
        return nil, string.format("subtype=%d, expected %d", subtype, expected_subtype)
    end
    if expected_record_size and record_size ~= expected_record_size then
        f:close()
        return nil, string.format("record_size=%d, expected %d",
            record_size, expected_record_size)
    end

    return {
        file         = f,
        magic        = magic,
        version      = version,
        test_type    = test_type,
        subtype      = subtype,
        record_size  = record_size,
        record_count = record_count,
        shared_count = shared_count,
        record_kind  = record_kind,
    }
end

-- Read the next record from an open inputs handle. Returns nil at EOF.
function M.read_record(reader)
    return reader.file:read(reader.record_size)
end

-- Close an open inputs handle.
function M.close_inputs(reader)
    if reader and reader.file then reader.file:close() end
end

-- ============================================================================
-- Outputs file: open + write header
-- ============================================================================
function M.open_outputs(path, test_type, subtype, record_size, record_count,
                        shared_count)
    local f = io.open(path, "wb")
    if not f then
        return nil, "could not open outputs (does the test_outputs/ directory exist?): " .. path
    end
    -- Layout per ALGORITHM_TEST_PLAN.md:
    --   0  4  magic         4  2  version       6  1  test_type    7  1  subtype
    --   8  2  record_size  10  4  record_count 14  4  shared_count
    --  18  1  record_kind  19 13  reserved
    f:write(M.TMTV_MAGIC)
    f:write(M.u16_le_bytes(M.TMTV_VERSION))
    f:write(string.char(test_type))
    f:write(string.char(subtype))
    f:write(M.u16_le_bytes(record_size))
    f:write(M.u32_le_bytes(record_count))
    f:write(M.u32_le_bytes(shared_count or 0))
    f:write(string.char(M.RECORD_KIND_FULL))
    f:write(string.rep("\0", 13))                      -- reserved
    return f
end

-- ============================================================================
-- Memory helpers (CPU RAM)
-- ============================================================================
-- Write a Lua string (binary) into a contiguous CPU memory range starting
-- at addr. Length = #data.
function M.write_range(addr, data)
    for i = 1, #data do
        memory.writebyte(addr + i - 1, string.byte(data, i))
    end
end

-- Read a contiguous CPU memory range and return as a Lua binary string.
function M.read_range(addr, length)
    local parts = {}
    for i = 0, length - 1 do
        parts[i + 1] = string.char(memory.readbyte(addr + i))
    end
    return table.concat(parts)
end

return M
