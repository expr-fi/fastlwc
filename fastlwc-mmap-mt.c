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

#include <omp.h>
#include "simd.h"

#define BLOCK_SIZE (32 * 32 * 1024)
#define SLICES_PER_BLOCK (int)(BLOCK_SIZE / sizeof(SIMD_VEC))

struct lwcount
{
	size_t lcount, wcount;
};

struct lwcount count_mt(unsigned char *restrict addr, size_t size)
{
	size_t lcount = 0,
	       wcount = 0;

	const size_t blocks = size / BLOCK_SIZE;
#pragma omp parallel if(blocks>1) reduction(+:lcount) reduction(+:wcount)
	{
		int tid = omp_get_thread_num(),
		    threads = omp_get_num_threads();
		for (size_t i = 0, block; (block = i * threads + tid) < blocks; ++i) {
			bool wcontinue = (block > 0
			                  ? !isspace(addr[block * BLOCK_SIZE - 1])
			                  : false);
			SIMD_VEC *vp = (SIMD_VEC*)&addr[block * BLOCK_SIZE];
			for (int j = 0; j < SLICES_PER_BLOCK; ++j, ++vp) {
				SIMD_VEC gt = SIMD_CMPGT8(*vp, SIMD_SET8(8)),
				         lt = SIMD_CMPGT8(SIMD_SET8(14), *vp),
				         eqnl = SIMD_CMPEQ8(*vp, SIMD_SET8('\n')),
				         eqsp = SIMD_CMPEQ8(*vp, SIMD_SET8(' '));
				lcount += SIMD_MASK_POPCNT(SIMD_CMASK8(eqnl));
				SIMD_VEC eqws = SIMD_OR(eqsp, SIMD_AND(gt, lt));
				SIMD_MASK wbits = ~SIMD_CMASK8(eqws);
				int words = SIMD_MASK_POPCNT(wbits)
				            - SIMD_MASK_POPCNT(wbits & (wbits << 1))
				            - (wcontinue && (wbits & 1));
				wcontinue = wbits & (1u << (sizeof(SIMD_VEC) - 1));
				wcount += words;
			}
		}
	}

	size_t remaining_size = size - blocks * BLOCK_SIZE;
	bool wcontinue = (blocks > 0
	                  ? !isspace(addr[blocks * BLOCK_SIZE - 1])
	                  : false);
	SIMD_VEC *vp = (SIMD_VEC*)&addr[blocks * BLOCK_SIZE];
	while (remaining_size >= sizeof(SIMD_VEC)) {
		SIMD_VEC gt = SIMD_CMPGT8(*vp, SIMD_SET8(8)),
		         lt = SIMD_CMPGT8(SIMD_SET8(14), *vp),
		         eqnl = SIMD_CMPEQ8(*vp, SIMD_SET8('\n')),
		         eqsp = SIMD_CMPEQ8(*vp, SIMD_SET8(' '));
		lcount += SIMD_MASK_POPCNT(SIMD_CMASK8(eqnl));
		SIMD_VEC eqws = SIMD_OR(eqsp, SIMD_AND(gt, lt));
		SIMD_MASK wbits = ~SIMD_CMASK8(eqws);
		int words = SIMD_MASK_POPCNT(wbits)
		            - SIMD_MASK_POPCNT(wbits & (wbits << 1))
		            - (wcontinue && (wbits & 1));
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
		fputs("usage: fastlwc-mmap-mt FILE\n", stderr);
		exit(EXIT_FAILURE);
	}

	int fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		perror("fastlwc-mmap-mt: open");
		exit(EXIT_FAILURE);
	}

	struct stat st;
	if (fstat(fd, &st)) {
		perror("fastlwc-mmap-mt: fstat");
		close(fd);
		exit(EXIT_FAILURE);
	}

	if (!S_ISREG(st.st_mode)) {
		fprintf(stderr, "fastlwc-mmap-mt: %s is not a regular file\n", argv[1]);
		close(fd);
		exit(EXIT_FAILURE);
	}

	void *addr = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED | MAP_NORESERVE, fd, 0);
	if (addr == MAP_FAILED) {
		perror("fastlwc-mmap-mt: mmap");
		close(fd);
		exit(EXIT_FAILURE);
	}

	struct lwcount lwc = count_mt(addr, st.st_size);

	printf(" %7zu %7zu %7zu %s\n", lwc.lcount, lwc.wcount, (size_t)st.st_size,
	                               argv[1]);

	munmap(addr, st.st_size);
	close(fd);
}
