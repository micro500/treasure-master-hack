#ifndef SIMD_H
#define SIMD_H

#include <immintrin.h>

#if defined(__GNUC__)
#define _mm256_set_m128i(vh, vl) \
	_mm256_castpd_si256( \
		_mm256_insertf128_pd( \
			_mm256_castsi256_pd(_mm256_castsi128_si256(vl)), \
			_mm_castsi128_pd(vh), \
			1 \
		) \
	)
#endif
//
//#define __mm256_srli_1(v) \
//	_mm256_or_si256( \
//		_mm256_srli_si256(v, 1), \
//		_mm256_slli_si256( \
//			_mm256_permute2x128_si256(v, _mm256_setzero_si256(), 0x21), \
//			15 \
//		) \
//	)


#endif // SIMD_H