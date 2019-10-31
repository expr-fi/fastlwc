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
#ifdef __AVX2__
		" avx2"
#else
		" e-avx2"
#endif
#if defined(__AVX512F__) && defined(__AVX512BW__)
	    " avx512"
#else
	    " e-avx512"
#endif
	);
	return 0;
}
