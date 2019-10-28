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

#define BUFSIZE (sizeof(SIMD_VEC) * 4096)

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fputs("usage: fastlwc FILE\n", stderr);
		exit(EXIT_FAILURE);
	}

	unsigned char *buf = aligned_alloc(sizeof(SIMD_VEC), BUFSIZE);
	if (!buf) {
		perror("fastlwc: alloc");
		exit(EXIT_FAILURE);
	}

	int fd = (strcmp(argv[1], "-") == 0 ? STDIN_FILENO : open(argv[1], O_RDONLY));
	if (fd < 0) {
		perror("fastlwc: open");
		exit(EXIT_FAILURE);
	}

	posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);

	ssize_t len;
	size_t lcount = 0,
	       wcount = 0,
	       ccount = 0,
	       rem = 0;
	bool wcontinue = false;

	SIMD_VEC *vp = (SIMD_VEC*)buf;
	while ((len = read(fd, buf + rem, BUFSIZE - rem))) {
		if (len < 0) {
			perror("fastlwc: read");
			exit(EXIT_FAILURE);
		}

		rem += len;
		ccount += len;
		vp = (SIMD_VEC*)buf;

		while (rem >= sizeof(SIMD_VEC)) {
			SIMD_VEC lws = SIMD_CMPGT8(SIMD_ADD8(*vp, SIMD_SET8(113)),
			                           SIMD_SET8(121)),
			         eqnl = SIMD_CMPEQ8(*vp, SIMD_SET8('\n')),
			         eqsp = SIMD_CMPEQ8(*vp, SIMD_SET8(' '));
			lcount += SIMD_MASK_POPCNT(SIMD_CMASK8(eqnl));
			SIMD_VEC eqws = SIMD_OR(eqsp, lws);
			SIMD_MASK wbits = ~SIMD_CMASK8(eqws);
			int words = SIMD_MASK_POPCNT(wbits & ~((wbits << 1) + wcontinue));
			wcontinue = wbits & (1ul << (sizeof(SIMD_VEC) - 1));
			wcount += words;

			rem -= sizeof(SIMD_VEC);
			vp++;
		}

		if (rem)
			memmove(buf, vp, rem);
	}

	unsigned char *p = (unsigned char*)vp;
	while (rem) {
		if (!isspace(*p)) {
			if (!wcontinue) {
				wcount++;
				wcontinue = true;
			}
		} else {
			if (*p == '\n')
				lcount++;
			wcontinue = false;
		}
		p++;
		rem--;
	}

	printf(" %7zu %7zu %7zu %s\n", lcount, wcount, ccount, argv[1]);

	close(fd);
	free(buf);
}
