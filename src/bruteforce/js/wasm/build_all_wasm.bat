@echo off
rem Run from src/bruteforce/wasm/ after activating emsdk (emsdk_env.bat)
call _build_one.bat tm_64_8          scalar
call _build_one.bat tm_ssse3_m128_8      ssse3
call _build_one.bat tm_ssse3_m128s_8     ssse3
call _build_one.bat tm_ssse3_m128_map_8  ssse3
call _build_one.bat tm_ssse3_m128s_map_8 ssse3
call _build_one.bat tm_ssse3_r128_8      ssse3
call _build_one.bat tm_ssse3_r128s_8     ssse3
call _build_one.bat tm_ssse3_r128_map_8  ssse3
call _build_one.bat tm_ssse3_r128s_map_8 ssse3
