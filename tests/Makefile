CFLAGS = -std=gnu11 -O0 -Wall -Wextra -fopenmp
CFLAGS_NOSIMD = $(CFLAGS) -DNO_SIMD
CFLAGS_SSE2   = $(CFLAGS) -msse -msse2
CFLAGS_SSSE3  = $(CFLAGS_SSE2) -msse3 -mssse3
CFLAGS_AVX    = $(CFLAGS_SSSE3) -msse4 -msse4.1 -msse4.2 -mavx -mpopcnt
CFLAGS_AVX2   = $(CFLAGS_AVX) -mavx2
CFLAGS_AVX512 = $(CFLAGS_AVX2) -mavx512f -mavx512bw

TESTS    = nosimd sse2 ssse3 avx avx-popcnt avx2 avx2-popcnt avx512 avx512-popcnt
BINARIES = fastlwc fastlwc-mmap fastlwc-mmap-mt
TARGETS  = $(foreach dir,$(TESTS),$(addprefix bin/$(dir)/,$(BINARIES)))
TARGETS += bin/bsd-wc bin/native-spec

test: $(TARGETS)
	./run-tests.sh $(shell bin/native-spec)

bin/nosimd/%: ../%.c ../simd.c ../simd.h
	@mkdir -p $(@D)
	$(CC) $(CFLAGS_NOSIMD) $< ../simd.c -o $@ $(LDFLAGS)

bin/sse2/%: ../%.c ../simd.c ../simd.c ../simd.h
	@mkdir -p $(@D)
	$(CC) $(CFLAGS_SSE2) $< ../simd.c -o $@ $(LDFLAGS)

bin/ssse3/%: ../%.c ../simd.c ../simd.h
	@mkdir -p $(@D)
	$(CC) $(CFLAGS_SSSE3) $< ../simd.c -o $@ $(LDFLAGS)

bin/avx/%: ../%.c ../simd.c ../simd.h
	@mkdir -p $(@D)
	$(CC) $(CFLAGS_AVX) $< ../simd.c -o $@ $(LDFLAGS)

bin/avx-popcnt/%: ../%.c ../simd.c ../simd.h
	@mkdir -p $(@D)
	$(CC) $(CFLAGS_AVX) -DSIMD_USE_POPCNT $< ../simd.c -o $@ $(LDFLAGS)

bin/avx2/%: ../%.c ../simd.c ../simd.h
	@mkdir -p $(@D)
	$(CC) $(CFLAGS_AVX2) $< ../simd.c -o $@ $(LDFLAGS)

bin/avx2-popcnt/%: ../%.c ../simd.c ../simd.h
	@mkdir -p $(@D)
	$(CC) $(CFLAGS_AVX2) -DSIMD_USE_POPCNT $< ../simd.c -o $@ $(LDFLAGS)

bin/avx512/%: ../%.c ../simd.c ../simd.h
	@mkdir -p $(@D)
	$(CC) $(CFLAGS_AVX512) $< ../simd.c -o $@ $(LDFLAGS)

bin/avx512-popcnt/%: ../%.c ../simd.c ../simd.h
	@mkdir -p $(@D)
	$(CC) $(CFLAGS_AVX512) -DSIMD_USE_POPCNT $< ../simd.c -o $@ $(LDFLAGS)

bin/bsd-wc: ../bsd-wc.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

bin/native-spec: native-spec.c
	@mkdir -p $(@D)
	$(CC) -march=native $< -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	test -d bin && rm -r bin || true
