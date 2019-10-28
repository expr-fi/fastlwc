#ifndef SIMD_H_
#define SIMD_H_

#include <inttypes.h>
#include <nmmintrin.h>


#if defined(__AVX512F__) && defined(__AVX512BW__)
#  include <immintrin.h>
#  define SIMD_VEC           __m512i
#  define SIMD_MASK          uint64_t
#  define SIMD_SET8          _mm512_set1_epi8
#  define SIMD_CMPGT8(X, Y)  _mm512_movm_epi8(_mm512_cmpgt_epi8_mask(X, Y))
#  define SIMD_CMPEQ8(X, Y)  _mm512_movm_epi8(_mm512_cmpeq_epi8_mask(X, Y))
#  define SIMD_ADD8          _mm512_add_epi8
#  define SIMD_AND           _mm512_and_si512
#  define SIMD_OR            _mm512_or_si512
#  define SIMD_CMASK8(X)     _cvtmask64_u64(_mm512_movepi8_mask(X))
#  define SIMD_MASK_POPCNT   _mm_popcnt_u64
#elif defined(__AVX2__)
#  include <immintrin.h>
#  define SIMD_VEC           __m256i
#  define SIMD_MASK          uint32_t
#  define SIMD_SET8          _mm256_set1_epi8
#  define SIMD_CMPGT8        _mm256_cmpgt_epi8
#  define SIMD_CMPEQ8        _mm256_cmpeq_epi8
#  define SIMD_ADD8          _mm256_add_epi8
#  define SIMD_AND           _mm256_and_si256
#  define SIMD_OR            _mm256_or_si256
#  define SIMD_CMASK8        _mm256_movemask_epi8
#  define SIMD_MASK_POPCNT   _mm_popcnt_u32
#else
#  include <emmintrin.h>
#  define SIMD_VEC           __m128i
#  define SIMD_MASK          uint16_t
#  define SIMD_SET8          _mm_set1_epi8
#  define SIMD_CMPGT8        _mm_cmpgt_epi8
#  define SIMD_CMPEQ8        _mm_cmpeq_epi8
#  define SIMD_ADD8          _mm_add_epi8
#  define SIMD_AND           _mm_and_si128
#  define SIMD_OR            _mm_or_si128
#  define SIMD_CMASK8        _mm_movemask_epi8
#  define SIMD_MASK_POPCNT   _mm_popcnt_u32
#endif

#endif // SIMD_H_
