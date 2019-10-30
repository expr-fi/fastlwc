CFLAGS = -std=gnu11 -O2 -Wall -Wextra -march=x86-64 -mpopcnt -fopenmp
CFLAGS_AVX2 = $(CFLAGS) -msse3 -mssse3 -msse4 -msse4.1 -msse4.2 -mavx -mavx2
CFLAGS_AVX512 = $(CFLAGS_AVX2) -mavx512f -mavx512bw

TARGETS = \
	x86-64/fastlwc x86-64/fastlwc-mmap x86-64/fastlwc-mmap-mt \
	avx2/fastlwc   avx2/fastlwc-mmap   avx2/fastlwc-mmap-mt   \
	avx512/fastlwc avx512/fastlwc-mmap avx512/fastlwc-mmap-mt \
	bin/bsd-wc bin/native-spec

test: $(TARGETS)
	./run-tests.sh $(shell bin/native-spec)

x86-64/%: ../%.c ../simd.h
	@mkdir -p x86-64
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

avx2/%: ../%.c ../simd.h
	@mkdir -p avx2
	$(CC) $(CFLAGS_AVX2) $< -o $@ $(LDFLAGS)

avx512/%: ../%.c ../simd.h
	@mkdir -p avx512
	$(CC) $(CFLAGS_AVX512) $< -o $@ $(LDFLAGS)

bin/bsd-wc: ../bsd-wc.c ../simd.h
	@mkdir -p bin
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

bin/native-spec: native-spec.c
	@mkdir -p bin
	$(CC) -march=native $< -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(TARGETS)
	for dir in x86-64 avx2 avx512 bin; do test -d $$dir && rmdir $$dir || true; done
