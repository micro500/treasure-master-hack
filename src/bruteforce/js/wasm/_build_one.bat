@echo off
rem Usage: _build_one.bat <impl_name> <scalar|ssse3>
set IMPL=%~1
set SIMD=
if "%~2"=="ssse3" set SIMD=-msimd128 -msse2 -mssse3
echo Building %IMPL%...
em++ -std=c++17 -O2 %SIMD% ^
  -include wasm_compat.h ^
  -include ../../cpu/%IMPL%.h ^
  -DWASM_IMPL_CLASS=%IMPL% ^
  -I . ^
  -I ../../../common ^
  -I ../../cpu ^
  ../../../common/alignment2.cpp ^
  ../../../common/rng_obj.cpp ^
  ../../../common/key_schedule.cpp ^
  ../../../common/tm_base.cpp ^
  ../../cpu/%IMPL%.cpp ^
  wasm_bindings.cpp ^
  -s MODULARIZE=1 ^
  -s EXPORT_NAME="TmModule" ^
  -s ALLOW_MEMORY_GROWTH=1 ^
  -s INITIAL_MEMORY=67108864 ^
  -s EXPORTED_FUNCTIONS="['_tm_wasm_init','_tm_wasm_run','_tm_wasm_compute_flags','_tm_wasm_test_expansion','_tm_wasm_test_bruteforce_data','_tm_wasm_test_bruteforce_checksum','_tm_wasm_test_algorithm','_tm_wasm_test_algorithm_chain','_tm_wasm_tracks_rng_state','_malloc','_free']" ^
  -s EXPORTED_RUNTIME_METHODS="['ccall','cwrap','HEAPU8']" ^
  -o %IMPL%.js
if errorlevel 1 (echo FAILED: %IMPL%) else (echo OK: %IMPL%)
