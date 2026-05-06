#pragma once
// Emscripten-compatible replacement for <immintrin.h>.
// Only SSE2 + SSSE3 are needed for the ssse3_* impls.
// Compile with: -msimd128 -msse2 -mssse3
#include <emmintrin.h>
#include <tmmintrin.h>
