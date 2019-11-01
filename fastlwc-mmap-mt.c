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
#include <string.h>

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
			lcount_state lstate = LCOUNT_INITIAL;
			wcount_state wstate = (block > 0 && !isspace(addr[block * BLOCK_SIZE - 1])
			                       ? WCOUNT_CONTINUE : WCOUNT_INITIAL);
			SIMD_VEC *vp = (SIMD_VEC*)&addr[block * BLOCK_SIZE];
			for (int j = 0; j < SLICES_PER_BLOCK; ++j, ++vp) {
				lcount += count_lines(*vp, &lstate);
				wcount += count_words(*vp, &wstate);
			}
			lcount += count_lines_final(&lstate);
			wcount += count_words_final(&wstate);
		}
	}

	size_t rem = size - blocks * BLOCK_SIZE;
	lcount_state lstate = LCOUNT_INITIAL;
	wcount_state wstate = (blocks > 0 && !isspace(addr[blocks * BLOCK_SIZE - 1])
	                       ? WCOUNT_CONTINUE : WCOUNT_INITIAL);
	SIMD_VEC *vp = (SIMD_VEC*)&addr[blocks * BLOCK_SIZE];
	while (rem >= sizeof(SIMD_VEC)) {
		lcount += count_lines(*vp, &lstate);
		wcount += count_words(*vp, &wstate);
		rem -= sizeof(SIMD_VEC);
		vp++;
	}

	if (rem > 0) {
		SIMD_VEC buf;
		memcpy(&buf, vp, rem);
		memset((char*)&buf + rem, ' ', sizeof(SIMD_VEC) - rem);
		lcount += count_lines(buf, &lstate);
		wcount += count_words(buf, &wstate);
	}

	lcount += count_lines_final(&lstate);
	wcount += count_words_final(&wstate);
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

	struct lwcount lwc;
	if (st.st_size > 0) {
		void *addr = mmap(NULL, st.st_size, PROT_READ,
		                  MAP_SHARED | MAP_NORESERVE, fd, 0);
		if (addr == MAP_FAILED) {
			perror("fastlwc-mmap: mmap");
			close(fd);
			exit(EXIT_FAILURE);
		}
		lwc = count_mt(addr, st.st_size);
		munmap(addr, st.st_size);
	} else
		lwc = (struct lwcount){ 0, 0 };

	printf(" %7zu %7zu %7zu %s\n", lwc.lcount, lwc.wcount, (size_t)st.st_size,
	                               argv[1]);
	close(fd);
}
