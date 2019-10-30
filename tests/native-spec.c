#include <stdio.h>

int main(void)
{
	puts(
	    "x86-64"
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
