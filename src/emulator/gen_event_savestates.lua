-- gen_event_savestates.lua
--
-- Replays the TAS at maximum speed, dropping a named savestate at each
-- frame listed in lib_test.savestates (the single source of truth).
-- Output filenames follow lib_test's convention: savestates/F<5-digit>_<name>.fcs
--
-- Usage:
--   1. Load the ROM and rewind the TAS movie to frame 0.
--   2. File -> Lua -> New Lua Script Window -> browse to this file -> Run.
--   3. Wait ~60 seconds. The .fcs files appear in savestates/.

local lib = dofile((function()
    local src = debug.getinfo(1, "S").source
    if src:sub(1, 1) == "@" then src = src:sub(2) end
    return (src:match("(.*[/\\])") or "") .. "lib_test.lua"
end)())

-- The savestates dir must exist before FCEUX's savestate.create(path) is
-- called — passing a path in a missing dir crashes the emulator on some
-- builds. Probe by writing a sentinel file; bail with a clear error if
-- the directory isn't there.
do
    local probe = lib.savestates_dir .. ".dir_probe"
    local f = io.open(probe, "wb")
    if not f then
        print(string.format(
            "ERROR: savestates directory does not exist: %s",
            lib.savestates_dir))
        print("Create it (e.g. `mkdir savestates`) and re-run.")
        return
    end
    f:close()
    os.remove(probe)
end

-- Build a chronological list from the registry.
local TARGETS = {}
for frame, _ in pairs(lib.savestates) do
    TARGETS[#TARGETS + 1] = frame
end
table.sort(TARGETS)

-- ============================================================================
-- Sanity: starting frame must be at or before the earliest save target.
-- ============================================================================
local start_frame = emu.framecount()
local first_save  = TARGETS[1]
if start_frame > first_save then
    print(string.format(
        "ERROR: starting frame %d is past the first save target %d. Rewind the movie first.",
        start_frame, first_save))
    return
end

print(string.format("[gen_event_savestates] starting at frame %d", start_frame))
print(string.format("[gen_event_savestates] %d savestates to write", #TARGETS))

-- ============================================================================
-- Drive the movie. At each target frame, save and continue.
-- ============================================================================
emu.speedmode("maximum")

for i, save_frame in ipairs(TARGETS) do
    while emu.framecount() < save_frame do
        emu.frameadvance()
    end

    local entry = lib.savestates[save_frame]
    local path  = entry.file
    local fname = path:match("([^/\\]+)$")
    local ss    = savestate.create(path)
    savestate.save(ss)
    if savestate.persist then savestate.persist(ss) end

    local f = io.open(path, "rb")
    if f then
        local size = f:seek("end")
        f:close()
        print(string.format("[gen_event_savestates] %2d/%d  saved %s  (%d bytes)",
            i, #TARGETS, fname, size))
    else
        print(string.format("[gen_event_savestates] %2d/%d  FAILED to write %s",
            i, #TARGETS, fname))
    end
end

emu.speedmode("normal")
print(string.format("[gen_event_savestates] done. %d savestates written.", #TARGETS))
