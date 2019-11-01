#ifndef SIMD_H_
#define SIMD_H_

#include <ctype.h>
#include <stdint.h>
#include <nmmintrin.h>

#ifdef SIMD_IMPL
#  define SIMD_INLINE
#else
#  define SIMD_INLINE inline
#endif

#if defined(__GNUC__) || defined(__clang__)
#  define popcnt32_fallback __builtin_popcount
#elif defined(__POPCNT__)
#  define popcnt32_fallback _mm_popcnt_u32
#else
SIMD_INLINE int popcnt32_fallback(uint32_t num)
{
	uint32_t c;
	for (c = 0; num; c++)
		num &= num - 1;
	return c;
}
#endif

#ifdef NO_SIMD
typedef unsigned char SIMD_VEC;
#elif defined(__AVX512F__) && defined(__AVX512BW__)
#  include <immintrin.h>
#  define SIMD_VEC                __m512i
#  define SIMD_MASK               uint64_t
#  define SIMD_SET8               _mm512_set1_epi8
#  define SIMD_CMPEQ8_MASK(a, b)  _cvtmask64_u64(_mm512_cmpeq_epi8_mask((a), (b)))
#  define SIMD_SHUFFLE            _mm512_shuffle_epi8
// _mm512_set_epi8 missing in GCC
#  define SIMD_SHUFSRC_WS()  \
	_mm512_setr_epi64(0x20, 0x0d0c0b0a0900, 0x20, 0x0d0c0b0a0900, \
	                  0x20, 0x0d0c0b0a0900, 0x20, 0x0d0c0b0a0900)
#  define SIMD_MASK_POPCNT        _mm_popcnt_u64
#elif defined(__AVX2__)
#  include <immintrin.h>
#  define SIMD_VEC                __m256i
#  define SIMD_MASK               uint32_t
#  define SIMD_SET8               _mm256_set1_epi8
#  define SIMD_CMPEQ8_MASK(a, b)  _mm256_movemask_epi8(_mm256_cmpeq_epi8((a), (b)))
#  define SIMD_SHUFFLE            _mm256_shuffle_epi8
#  define SIMD_SHUFSRC_WS()  \
	_mm256_setr_epi8(' ', 0, 0, 0, 0, 0, 0, 0, 0, '\t', '\n', '\v', '\f', '\r', 0, 0, \
	                 ' ', 0, 0, 0, 0, 0, 0, 0, 0, '\t', '\n', '\v', '\f', '\r', 0, 0)
#  define SIMD_MASK_POPCNT        _mm_popcnt_u32
#elif defined(__SSSE3__)
#  include <emmintrin.h>
#  define SIMD_VEC                __m128i
#  define SIMD_MASK               uint16_t
#  define SIMD_SET8               _mm_set1_epi8
#  define SIMD_CMPEQ8_MASK(a, b)  _mm_movemask_epi8(_mm_cmpeq_epi8((a), (b)))
#  define SIMD_SHUFFLE            _mm_shuffle_epi8
#  define SIMD_SHUFSRC_WS()  \
	_mm_setr_epi8(' ', 0, 0, 0, 0, 0, 0, 0, 0, '\t', '\n', '\v', '\f', '\r', 0, 0)
#  define SIMD_MASK_POPCNT        popcnt32_fallback
#elif defined(__SSE2__)
#  include <emmintrin.h>
#  define SIMD_VEC                __m128i
#  define SIMD_MASK               uint16_t
#  define SIMD_SET8               _mm_set1_epi8
#  define SIMD_CMPEQ8             _mm_cmpeq_epi8
#  define SIMD_CMPGT8             _mm_cmpgt_epi8
#  define SIMD_ADD8               _mm_add_epi8
#  define SIMD_OR                 _mm_or_si128
#  define SIMD_CMASK8             _mm_movemask_epi8
#  define SIMD_MASK_POPCNT        popcnt32_fallback
#else
typedef unsigned char SIMD_VEC;
#  ifndef NO_SIMD
#    define NO_SIMD 1
#  endif
#endif

typedef int wcount_state_t;
#define WCOUNT_BOUNDARY 0
#define WCOUNT_CONTINUE 1

#ifdef NO_SIMD
SIMD_INLINE int count_lines(SIMD_VEC vec)
{
	return vec == '\n';
}
SIMD_INLINE int count_words(SIMD_VEC vec, wcount_state_t *state)
{
	if (!isspace(vec)) {
		if (*state == WCOUNT_BOUNDARY) {
			*state = WCOUNT_CONTINUE;
			return 1;
		}
	} else
		*state = WCOUNT_BOUNDARY;
	return 0;
}
#elif (defined(__AVX512F__) && defined(__AVX512BW__)) \
      || defined(__AVX2__) || defined(__SSSE3__)
SIMD_INLINE int count_lines(SIMD_VEC vec)
{
	return SIMD_MASK_POPCNT(SIMD_CMPEQ8_MASK(vec, SIMD_SET8('\n')));
}
SIMD_INLINE int count_words(SIMD_VEC vec, wcount_state_t *state)
{
	SIMD_MASK wbits = ~SIMD_CMPEQ8_MASK(SIMD_SHUFFLE(SIMD_SHUFSRC_WS(), vec), vec);
	int words = SIMD_MASK_POPCNT(wbits & ~((wbits << 1) + *state));
	*state = wbits >> (sizeof(SIMD_VEC) - 1);
	return words;
}
#elif defined(__SSE2__)
SIMD_INLINE int count_lines(SIMD_VEC vec)
{
	return SIMD_MASK_POPCNT(SIMD_CMASK8(SIMD_CMPEQ8(vec, SIMD_SET8('\n'))));
}
SIMD_INLINE int count_words(SIMD_VEC vec, wcount_state_t *state)
{
	SIMD_VEC lws = SIMD_CMPGT8(SIMD_ADD8(vec, SIMD_SET8(113)), SIMD_SET8(121)),
	         eqsp = SIMD_CMPEQ8(vec, SIMD_SET8(' '));
	SIMD_MASK wbits = ~SIMD_CMASK8(SIMD_OR(eqsp, lws));
	int words = SIMD_MASK_POPCNT(wbits & ~((wbits << 1) + *state));
	*state = wbits >> (sizeof(SIMD_VEC) - 1);
	return words;
}
#else
#  error Invalid state
#endif

#endif // SIMD_H_
