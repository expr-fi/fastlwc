CFLAGS += -std=gnu11 -Wall -Wextra -pedantic -march=native -Ofast
TARGETS = bin/bsd-wc bin/fastlwc bin/fastlwc-mmap bin/fastlwc-mmap-mt

all: $(TARGETS)

bin/fastlwc-mmap-mt: fastlwc-mmap-mt.c simd.c simd.h
	@mkdir -p bin
	$(CC) $(CFLAGS) -fopenmp $< simd.c -o $@ $(LDFLAGS)

bin/%: %.c simd.c simd.h
	@mkdir -p bin
	$(CC) $(CFLAGS) $< simd.c -o $@ $(LDFLAGS)

.PHONY: tests
tests:
	cd tests/ && $(MAKE)

.PHONY: clean
clean:
	cd tests/ && $(MAKE) clean
	test -d bin && rm -r bin || true
