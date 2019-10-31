CFLAGS = -std=gnu11 -O0 -Wall -Wextra -fopenmp
CFLAGS_NOSIMD = $(CFLAGS) -DNO_SIMD
CFLAGS_SSE2   = $(CFLAGS) -msse -msse2
CFLAGS_SSSE3  = $(CFLAGS_SSE2) -msse3 -mssse3
CFLAGS_AVX2   = $(CFLAGS_SSSE3) -msse4 -msse4.1 -msse4.2 -mavx -mavx2
CFLAGS_AVX512 = $(CFLAGS_AVX2) -mavx512f -mavx512bw

TESTS    = nosimd sse2 ssse3 avx2 avx512
BINARIES = fastlwc fastlwc-mmap fastlwc-mmap-mt
TARGETS  = $(foreach dir,$(TESTS),$(addprefix $(dir)/,$(BINARIES)))
TARGETS += bin/bsd-wc bin/native-spec
DIRS     = $(TESTS) bin

test: $(TARGETS)
	./run-tests.sh $(shell bin/native-spec)

nosimd/%: ../%.c ../simd.c ../simd.h
	@mkdir -p nosimd
	$(CC) $(CFLAGS_NOSIMD) $< ../simd.c -o $@ $(LDFLAGS)

sse2/%: ../%.c ../simd.c ../simd.c ../simd.h
	@mkdir -p sse2
	$(CC) $(CFLAGS_SSE2) $< ../simd.c -o $@ $(LDFLAGS)

ssse3/%: ../%.c ../simd.c ../simd.h
	@mkdir -p ssse3
	$(CC) $(CFLAGS_SSSE3) $< ../simd.c -o $@ $(LDFLAGS)

avx2/%: ../%.c ../simd.c ../simd.h
	@mkdir -p avx2
	$(CC) $(CFLAGS_AVX2) $< ../simd.c -o $@ $(LDFLAGS)

avx512/%: ../%.c ../simd.c ../simd.h
	@mkdir -p avx512
	$(CC) $(CFLAGS_AVX512) $< ../simd.c -o $@ $(LDFLAGS)

bin/bsd-wc: ../bsd-wc.c
	@mkdir -p bin
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

bin/native-spec: native-spec.c
	@mkdir -p bin
	$(CC) -march=native $< -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(TARGETS)
	for dir in $(DIRS); do test -d $$dir && rmdir $$dir || true; done
