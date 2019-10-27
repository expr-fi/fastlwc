CFLAGS += -std=gnu11 -Wall -Wextra -pedantic -march=native -Ofast

all: bsd-wc fastlwc fastlwc-mmap fastlwc-mmap-mt

fastlwc-mmap-mt: fastlwc-mmap-mt.c
	$(CC) $(CFLAGS) -fopenmp $< -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f bsd-wc fastlwc fastlwc-mmap fastlwc-mmap-mt
