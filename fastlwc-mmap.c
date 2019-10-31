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

#include "simd.h"

struct lwcount
{
	size_t lcount, wcount;
};

struct lwcount count(unsigned char *restrict addr, size_t rem)
{
	size_t lcount = 0,
	       wcount = 0;
	wcount_state_t state = WCOUNT_BOUNDARY;

	SIMD_VEC *vp = (SIMD_VEC*)addr;
	while (rem >= sizeof(SIMD_VEC)) {
		wcount += count_words(*vp, &state);
		lcount += count_lines(*vp);
		rem -= sizeof(SIMD_VEC);
		vp++;
	}

	if (rem > 0) {
		SIMD_VEC buf;
		memcpy(&buf, vp, rem);
		memset((char*)&buf + rem, ' ', sizeof(SIMD_VEC) - rem);
		wcount += count_words(buf, &state);
		lcount += count_lines(buf);
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

	struct lwcount lwc;
	if (st.st_size > 0) {
		void *addr = mmap(NULL, st.st_size, PROT_READ,
		                  MAP_SHARED | MAP_NORESERVE, fd, 0);
		if (addr == MAP_FAILED) {
			perror("fastlwc-mmap: mmap");
			close(fd);
			exit(EXIT_FAILURE);
		}
		lwc = count(addr, st.st_size);
		munmap(addr, st.st_size);
	} else
		lwc = (struct lwcount){ 0, 0 };

	printf(" %7zu %7zu %7zu %s\n", lwc.lcount, lwc.wcount, (size_t)st.st_size,
	                               argv[1]);
	close(fd);
}
