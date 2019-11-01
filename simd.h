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
#elif defined(__AVX__) || defined(__SSSE3__) || defined(__SSE2__)
#  include <emmintrin.h>
#  include <immintrin.h>
#  include <tmmintrin.h>
#  define SIMD_VEC                __m128i
#  define SIMD_MASK               uint16_t
#  define SIMD_SET8               _mm_set1_epi8
#  define SIMD_CMPEQ8_MASK(a, b)  _mm_movemask_epi8(_mm_cmpeq_epi8((a), (b)))
#  define SIMD_CMPEQ8             _mm_cmpeq_epi8
#  define SIMD_CMPGT8             _mm_cmpgt_epi8
#  define SIMD_SHLB               _mm_bslli_si128
#  define SIMD_ADD8               _mm_add_epi8
#  define SIMD_ADD64              _mm_add_epi64
#  define SIMD_SUB8               _mm_sub_epi8
#  define SIMD_SAD8               _mm_sad_epu8
#  define SIMD_OR                 _mm_or_si128
#  define SIMD_XOR                _mm_xor_si128
#  define SIMD_ANDNOT             _mm_andnot_si128
#  define SIMD_CMASK8             _mm_movemask_epi8
#  define SIMD_SETZERO            _mm_setzero_si128
#  if defined(__AVX__) || defined(__SSSE3__)
#    define SIMD_SHUFFLE            _mm_shuffle_epi8
#    define SIMD_SHUFSRC_WS()  \
	   _mm_setr_epi8(' ', 0, 0, 0, 0, 0, 0, 0, 0, '\t', '\n', '\v', '\f', '\r', 0, 0)
#  endif
#  if defined(__AVX__) || defined(__POPCNT__)
#    define SIMD_MASK_POPCNT        _mm_popcnt_u32
#  endif
#else
typedef unsigned char SIMD_VEC;
#  define NO_SIMD 1
#endif

#if defined(NO_SIMD)
typedef int lcount_state;
typedef int wcount_state;
#define WCOUNT_INITIAL 0
#define WCOUNT_CONTINUE 1
#define LCOUNT_INITIAL 0

SIMD_INLINE uint64_t count_lines_final(lcount_state *state) { (void)state; return 0; }
SIMD_INLINE uint64_t count_words_final(wcount_state *state) { (void)state; return 0; }
SIMD_INLINE int count_lines(SIMD_VEC vec, lcount_state *state)
{
	(void)state;
	return vec == '\n';
}
SIMD_INLINE int count_words(SIMD_VEC vec, wcount_state *state)
{
	if (!isspace(vec)) {
		if (*state == WCOUNT_INITIAL) {
			*state = WCOUNT_CONTINUE;
			return 1;
		}
	} else
		*state = WCOUNT_INITIAL;
	return 0;
}
#elif defined(SIMD_MASK_POPCNT)
typedef int lcount_state;
typedef int wcount_state;
#define WCOUNT_INITIAL 0
#define WCOUNT_CONTINUE 1
#define LCOUNT_INITIAL 0

SIMD_INLINE uint64_t count_lines_final(lcount_state *state) { (void)state; return 0; }
SIMD_INLINE uint64_t count_words_final(wcount_state *state) { (void)state; return 0; }
SIMD_INLINE int count_lines(SIMD_VEC vec, lcount_state *state)
{
	(void)state;
	return SIMD_MASK_POPCNT(SIMD_CMPEQ8_MASK(vec, SIMD_SET8('\n')));
}
SIMD_INLINE int count_words(SIMD_VEC vec, wcount_state *state)
{
	SIMD_MASK wbits = ~SIMD_CMPEQ8_MASK(SIMD_SHUFFLE(SIMD_SHUFSRC_WS(), vec), vec);
	int words = SIMD_MASK_POPCNT(wbits & ~((wbits << 1) + *state));
	*state = wbits >> (sizeof(SIMD_VEC) - 1);
	return words;
}
#else
typedef struct {
	SIMD_VEC vlcount, lcount;
	short iterations;
} lcount_state;
typedef struct {
	SIMD_VEC vwcount, wcount;
	int iterations, wcontinue;
} wcount_state;
#define LCOUNT_INITIAL (lcount_state){ SIMD_SETZERO(), SIMD_SETZERO(), 0 }
#define WCOUNT_INITIAL (wcount_state){ SIMD_SETZERO(), SIMD_SETZERO(), 0, 0 }
#define WCOUNT_CONTINUE (wcount_state){ SIMD_SETZERO(), SIMD_SETZERO(), 0, 0xFF }

SIMD_INLINE uint64_t count_lines_final(lcount_state *state)
{
	state->lcount = SIMD_ADD64(state->lcount,
	                           SIMD_SAD8(state->vlcount, SIMD_SETZERO()));
	state->vlcount = SIMD_SETZERO();
	state->iterations = 0;
	int64_t *p = (int64_t*)&state->lcount;
	return p[0] + p[1];
}
SIMD_INLINE uint64_t count_words_final(wcount_state *state)
{
	state->wcount = SIMD_ADD64(state->wcount,
	                           SIMD_SAD8(state->vwcount, SIMD_SETZERO()));
	state->vwcount = SIMD_SETZERO();
	state->iterations = 0;
	int64_t *p = (int64_t*)&state->wcount;
	return p[0] + p[1];
}

SIMD_INLINE int count_lines(SIMD_VEC vec, lcount_state *state)
{
	state->vlcount = SIMD_SUB8(state->vlcount, SIMD_CMPEQ8(vec, SIMD_SET8('\n')));
	state->iterations++;
	if (state->iterations == 255) {
		state->lcount = SIMD_ADD64(state->lcount,
		                           SIMD_SAD8(state->vlcount, SIMD_SETZERO()));
		state->vlcount = SIMD_SETZERO();
		state->iterations = 0;
	}
	return 0;
}
SIMD_INLINE int count_words(SIMD_VEC vec, wcount_state *state)
{
#ifdef SIMD_SHUFFLE
	SIMD_VEC eqws = SIMD_CMPEQ8(SIMD_SHUFFLE(SIMD_SHUFSRC_WS(), vec), vec);
#else
	SIMD_VEC lws = SIMD_CMPGT8(SIMD_ADD8(vec, SIMD_SET8(113)), SIMD_SET8(121)),
	         eqws = SIMD_OR(lws, SIMD_CMPEQ8(vec, SIMD_SET8(' ')));  
#endif
	SIMD_VEC nonws = SIMD_CMPEQ8(eqws, SIMD_SETZERO()),
	         tmp =  SIMD_OR(SIMD_SHLB(nonws, 1),
	                        _mm_insert_epi16(SIMD_SETZERO(), state->wcontinue, 0)),
	         wstarts = SIMD_ANDNOT(tmp, nonws);
	
	state->vwcount = SIMD_SUB8(state->vwcount, wstarts);
	state->wcontinue = _mm_extract_epi16(nonws, 7) >> 8;
	state->iterations++;
	if (state->iterations == 255) {
		state->wcount = SIMD_ADD64(state->wcount,
		                           SIMD_SAD8(state->vwcount, SIMD_SETZERO()));
		state->vwcount = SIMD_SETZERO();
		state->iterations = 0;
	}
	return 0;
}
#endif

#endif // SIMD_H_
