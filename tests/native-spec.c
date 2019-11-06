#include <stdio.h>

int main(void)
{
	puts(
		"nosimd"
#ifdef __SSE2__
		" sse2"
#else
		" e-sse2" // SDE may not be able to emulate these anyway...
#endif
#ifdef __SSSE3__
		" ssse3"
#else
		" e-ssse3" // SDE may not be able to emulate these anyway...
#endif
#ifdef __AVX__
		" avx avx-popcnt"
#else
		" e-avx e-avx-popcnt"
#endif
#ifdef __AVX2__
		" avx2 avx2-popcnt"
#else
		" e-avx2 e-avx2-popcnt"
#endif
#if defined(__AVX512F__) && defined(__AVX512BW__)
	    " avx512 avx512-popcnt"
#else
	    " e-avx512 e-avx512-popcnt"
#endif
	);
	return 0;
}
