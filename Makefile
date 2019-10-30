CFLAGS += -std=gnu11 -Wall -Wextra -pedantic -march=native -Ofast
TARGETS = bin/bsd-wc bin/fastlwc bin/fastlwc-mmap bin/fastlwc-mmap-mt

all: $(TARGETS)

bin/fastlwc-mmap-mt: fastlwc-mmap-mt.c simd.h
	@mkdir -p bin
	$(CC) $(CFLAGS) -fopenmp $< -o $@ $(LDFLAGS)

bin/%: %.c simd.h
	@mkdir -p bin
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

.PHONY: tests
tests:
	cd tests/ && make

.PHONY: clean
clean:
	cd tests/ && make clean
	rm -f $(TARGETS)
	test -d bin && rmdir bin || true
