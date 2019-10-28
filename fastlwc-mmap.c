#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "simd.h"

struct lwcount
{
	size_t lcount, wcount;
};

struct lwcount count(unsigned char *restrict addr, size_t remaining_size)
{
	size_t lcount = 0,
	       wcount = 0;
	bool wcontinue = false;

	SIMD_VEC *vp = (SIMD_VEC*)addr;
	while (remaining_size >= sizeof(SIMD_VEC)) {
		SIMD_VEC lws = SIMD_CMPGT8(SIMD_ADD8(*vp, SIMD_SET8(113)),
		                           SIMD_SET8(121)),
		         eqnl = SIMD_CMPEQ8(*vp, SIMD_SET8('\n')),
		         eqsp = SIMD_CMPEQ8(*vp, SIMD_SET8(' '));
		lcount += SIMD_MASK_POPCNT(SIMD_CMASK8(eqnl));
		SIMD_VEC eqws = SIMD_OR(eqsp, lws);
		SIMD_MASK wbits = ~SIMD_CMASK8(eqws);
		int words = SIMD_MASK_POPCNT(wbits & ~((wbits << 1) + wcontinue));
		wcontinue = wbits & (1u << (sizeof(SIMD_VEC) - 1));
		wcount += words;

		remaining_size -= sizeof(SIMD_VEC);
		vp++;
	}

	addr = (unsigned char*)vp;
	while (remaining_size) {
		if (!isspace(*addr)) {
			if (!wcontinue) {
				wcount++;
				wcontinue = true;
			}
		} else {
			if (*addr == '\n')
				lcount++;
			wcontinue = false;
		}
		addr++;
		remaining_size--;
	}

	return (struct lwcount){ lcount, wcount };
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fputs("usage: fastlwc-mmap FILE\n", stderr);
		exit(EXIT_FAILURE);
	}

	int fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		perror("fastlwc-mmap: open");
		exit(EXIT_FAILURE);
	}

	struct stat st;
	if (fstat(fd, &st)) {
		perror("fastlwc-mmap: fstat");
		close(fd);
		exit(EXIT_FAILURE);
	}

	if (!S_ISREG(st.st_mode)) {
		fprintf(stderr, "fastlwc-mmap: %s is not a regular file\n", argv[1]);
		close(fd);
		exit(EXIT_FAILURE);
	}

	void *addr = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED | MAP_NORESERVE,
	                  fd, 0);
	if (addr == MAP_FAILED) {
		perror("fastlwc-mmap: mmap");
		close(fd);
		exit(EXIT_FAILURE);
	}

	struct lwcount lwc = count(addr, st.st_size);

	printf(" %7zu %7zu %7zu %s\n", lwc.lcount, lwc.wcount, (size_t)st.st_size,
	                               argv[1]);

	munmap(addr, st.st_size);
	close(fd);
}
