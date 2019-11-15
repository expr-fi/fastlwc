#ifndef SIMD_EXT_H
#define SIMD_EXT_H

#include "simd.h"

#if !defined(NO_SIMD) && defined(__AVX512F__) && defined(__AVX512BW__)
#define simd_mask_init()     _cvtu64_mask64(0)
#define simd_mask_any(a)     (!simd_mask_none(a))
#define simd_xor(a, b)       _mm512_xor_si512((a), (b))
#define simd_and(a, b)       _mm512_and_si512((a), (b))
#define simd_mask_or(a, b)   _kor_mask64((a), (b))
#define simd_mask_and(a, b)  _kand_mask64((a), (b))
#define simd_shuffle(a, b)   _mm512_shuffle_epi8((a), (b))
__mmask64 simd_mask_shl1_from(__mmask64 a, __mmask64 b)
{
	return _kor_mask64(_kshiftli_mask64(a, 1), _kshiftri_mask64(b, 63));
}
__mmask64 simd_mask_shl2_from(__mmask64 a, __mmask64 b)
{
	return _kor_mask64(_kshiftli_mask64(a, 2), _kshiftri_mask64(b, 62));
}
__mmask64 simd_mask_shr1_from(__mmask64 a, __mmask64 b)
{
	return _kor_mask64(_kshiftri_mask64(a, 1), _kshiftli_mask64(b, 63));
}
__mmask64 simd_mask_shr2_from(__mmask64 a, __mmask64 b)
{
	return _kor_mask64(_kshiftri_mask64(a, 2), _kshiftli_mask64(b, 62));
}
unsigned char simd_mask_none(simd_mask m)
{
	return _ktestz_mask64_u8(m, m);
}
static inline simd_vector simd_shl2_from_i8(__m512i a, __m512i b)
{
	__m512i idx = _mm512_setr_epi64(14, 15, 0, 1, 2, 3, 4, 5);
	return _mm512_alignr_epi8(a, _mm512_permutex2var_epi64(a, idx, b), 14);
}
static inline simd_vector simd_shr1_from_i8(__m512i a, __m512i b)
{
	__m512i idx = _mm512_setr_epi64(2, 3, 4, 5, 6, 7, 8, 9);
	return _mm512_alignr_epi8(_mm512_permutex2var_epi64(a, idx, b), a, 1);
}
static inline simd_vector simd_shr2_from_i8(__m512i a, __m512i b)
{
	__m512i idx = _mm512_setr_epi64(2, 3, 4, 5, 6, 7, 8, 9);
	return _mm512_alignr_epi8(_mm512_permutex2var_epi64(a, idx, b), a, 2);
}
static inline __m512i simd_set128_i64(int64_t a, int64_t b)
{
	return _mm512_set_epi64(a, b, a, b, a, b, a, b);
}
static inline _Bool has_multibyte_sequence(__m512i vec)
{
	// return true if MSB is set on any of the bytes
	__mmask64 msb = _mm512_cmplt_epi8_mask(vec, _mm512_setzero_si512());
	return !_ktestz_mask64_u8(msb, msb);
}
static inline simd_mask simd_mask_expandl3(__mmask64 mbws, __mmask64 *rollover)
{
	*rollover = _kor_mask64(_kshiftri_mask64(mbws, 63),
	                        _kshiftri_mask64(mbws, 62));
	return _kor_mask64(mbws, _kor_mask64(_kshiftli_mask64(mbws, 1),
	                                     _kshiftli_mask64(mbws, 2)));
}
//endif AVX-512
#elif !defined(NO_SIMD) && defined(__AVX2__)
// no opmask in AVX2 and below, use value vectors instead
#define simd_mask_init()     simd_setzero()
#define simd_mask_any(a)     _mm256_movemask_epi8(a)
#define simd_mask_none(a)    (!simd_mask_any(a))
#define simd_xor(a, b)       _mm256_xor_si256((a), (b))
#define simd_and(a, b)       _mm256_and_si256((a), (b))
#define simd_mask_or(a, b)   _mm256_or_si256((a), (b))
#define simd_mask_and(a, b)  _mm256_and_si256((a), (b))
#define simd_shuffle(a, b)   _mm256_shuffle_epi8((a), (b))
#define simd_mask_shl1_from  simd_shl1_from_i8
#define simd_mask_shl2_from  simd_shl2_from_i8
#define simd_mask_shr1_from  simd_shr1_from_i8
#define simd_mask_shr2_from  simd_shr2_from_i8
static inline simd_vector simd_shl2_from_i8(simd_vector a, simd_vector b)
{
	return _mm256_alignr_epi8(a, _mm256_permute2x128_si256(b, a, 0x21), 14);
}
static inline simd_vector simd_shr1_from_i8(simd_vector a, simd_vector b)
{
	return _mm256_alignr_epi8(_mm256_permute2x128_si256(a, b, 0x21), a, 1);
}
static inline simd_vector simd_shr2_from_i8(simd_vector a, simd_vector b)
{
	return _mm256_alignr_epi8(_mm256_permute2x128_si256(a, b, 0x21), a, 2);
}
static inline simd_vector simd_set128_i64(int64_t a, int64_t b)
{
	return _mm256_set_epi64x(a, b, a, b);
}
static inline _Bool has_multibyte_sequence(simd_vector vec)
{
	// return true if MSB is set on any of the bytes
	return _mm256_movemask_epi8(vec);
}
// this hilariously specific function exists because of our wacky masks
// expand any set bit in mask to it's left-adjacent two places, store rollover
static inline simd_mask simd_mask_expandl3(simd_mask mbws, simd_mask *rollover)
{
	*rollover = simd_mask_or(simd_mask_shl1_from(simd_mask_init(), mbws),
	                         simd_mask_shl2_from(simd_mask_init(), mbws));
	simd_mask mbws1 = simd_mask_shl1_from(mbws, simd_mask_init()),
	          mbws2 = simd_mask_shl2_from(mbws, simd_mask_init());
	return simd_mask_or(mbws, simd_mask_or(mbws1, mbws2));
}
// endif AVX2
#elif !defined(NO_SIMD) && (defined(__SSE2__) || defined(_M_X64) || _M_IX86_FP == 2)
#define simd_mask_init()     simd_setzero()
#define simd_mask_any(a)     _mm_movemask_epi8(a)
#define simd_mask_none(a)    (!simd_mask_any(a))
#define simd_xor(a, b)       _mm_xor_si128((a), (b))
#define simd_and(a, b)       _mm_and_si128((a), (b))
#define simd_mask_or(a, b)   _mm_or_si128((a), (b))
#define simd_mask_and(a, b)  _mm_and_si128((a), (b))
// actually requires SSSE3 but whatever, could be replaced cmp&or for SSE2
#define simd_shuffle(a, b)   _mm_shuffle_epi8((a), (b))
#define simd_mask_shl1_from  simd_shl1_from_i8
#define simd_mask_shl2_from  simd_shl2_from_i8
#define simd_mask_shr1_from  simd_shr1_from_i8
#define simd_mask_shr2_from  simd_shr2_from_i8
static inline simd_vector simd_shl2_from_i8(simd_vector a, simd_vector b)
{
	return _mm_alignr_epi8(a, b, 14);
}
static inline simd_vector simd_shr1_from_i8(simd_vector a, simd_vector b)
{
	return _mm_alignr_epi8(b, a, 1);
}
static inline simd_vector simd_shr2_from_i8(simd_vector a, simd_vector b)
{
	return _mm_alignr_epi8(b, a, 2);
}
static inline simd_vector simd_set128_i64(int64_t a, int64_t b)
{
	return _mm_set_epi64x(a, b);
}
static inline _Bool has_multibyte_sequence(simd_vector vec)
{
	return _mm_movemask_epi8(vec);
}
static inline simd_mask simd_mask_expandl3(simd_mask mbws, simd_mask *rollover)
{
	*rollover = simd_mask_or(simd_mask_shl1_from(simd_mask_init(), mbws),
	                         simd_mask_shl2_from(simd_mask_init(), mbws));
	simd_mask mbws1 = simd_mask_shl1_from(mbws, simd_mask_init()),
	          mbws2 = simd_mask_shl2_from(mbws, simd_mask_init());
	return simd_mask_or(mbws, simd_mask_or(mbws1, mbws2));
}
#else
// Yeah, this is out of scope
#error NO_SIMD
#endif

// xa, xb must be xor'd with '\x80' in advance of calling this function
// after all, all the bytes of interest have the MSB set, which makes
// the shuffle instruction pretty useless...
static inline simd_mask simd_cmpws_xmb_i8_mask(simd_vector xa, simd_vector xb)
{
	simd_mask ws = simd_mask_init();

	// let's make sure any further processing is absolutely necessary
	// (i.e. check for any \xe1, \xe2 or \xe3 of interest)
	{
		simd_vector shuffle = simd_set128_i64(0, 0x636261ff);
		simd_mask e123 = simd_cmpeq_i8_mask(simd_shuffle(shuffle, xa), xa);
		if (simd_mask_none(e123))
			return ws;
	}

	// offset towards beginning
	simd_vector xa1 = simd_shr1_from_i8(xa, xb),
	            xa2 = simd_shr2_from_i8(xa, xb);

	// check for U+1680 Ogham Space Mark \xe1 \x9a \x80 (yeah...)
	{
		simd_mask p0 = simd_cmpeq_i8_mask(xa, simd_set_i8('\x61')),
		          p1 = simd_cmpeq_i8_mask(xa1, simd_set_i8('\x1a')),
		          p2 = simd_cmpeq_i8_mask(xa2, simd_setzero());
		ws = simd_mask_or(ws, simd_mask_and(p0, simd_mask_and(p1, p2)));
	}

	// check for U+3000 Ideographic Space (\xe3 \x80 \x80)
	{
		simd_mask p0 = simd_cmpeq_i8_mask(xa, simd_set_i8('\x63')),
		          p1 = simd_cmpeq_i8_mask(xa1, simd_setzero()),
		          p2 = simd_cmpeq_i8_mask(xa2, simd_setzero());
		ws = simd_mask_or(ws, simd_mask_and(p0, simd_mask_and(p1, p2)));
	}

	// all the rest start with \xe2
	simd_mask p0 = simd_cmpeq_i8_mask(xa, simd_set_i8('\x62'));

	// check for U+205F Medium Mathematical Space (\xe2 \x81 \x9f)
	{
		simd_mask p1 = simd_cmpeq_i8_mask(xa1, simd_set_i8('\x01')),
		          p2 = simd_cmpeq_i8_mask(xa2, simd_set_i8('\x1f'));
		ws = simd_mask_or(ws, simd_mask_and(p0, simd_mask_and(p1, p2)));
	}

	// the rest start with \xe2 \x80
	simd_mask p0p1 = simd_mask_and(p0, simd_cmpeq_i8_mask(xa1, simd_setzero()));

	// check for U+2000–U+2006, U+2008–U+200A
	{
		simd_vector shuffle = simd_set128_i64(0x0a0908, 0x06050403020100);
		simd_mask p2 = simd_cmpeq_i8_mask(simd_shuffle(shuffle, xa2), xa2);
		ws = simd_mask_or(ws, simd_mask_and(p0p1, p2));
	}

	// check for U+2000 (again...), U+2028–U+2029
	{
		simd_vector shuffle = simd_set128_i64(0x2928, 0);
		simd_mask p2 = simd_cmpeq_i8_mask(simd_shuffle(shuffle, xa2), xa2);
		ws = simd_mask_or(ws, simd_mask_and(p0p1, p2));
	}

	return ws;
}

#endif
