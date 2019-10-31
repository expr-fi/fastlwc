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

	SIMD_VEC *buf = aligned_alloc(sizeof(SIMD_VEC), BUFSIZE);
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
	wcount_state_t state = WCOUNT_BOUNDARY;

	while ((len = read(fd, (char*)buf + rem, BUFSIZE - rem))) {
		if (len < 0) {
			perror("fastlwc: read");
			exit(EXIT_FAILURE);
		}

		rem += len;
		ccount += len;

		SIMD_VEC *vp = buf;
		while (rem >= sizeof(SIMD_VEC)) {
			wcount += count_words(*vp, &state);
			lcount += count_lines(*vp);

			rem -= sizeof(SIMD_VEC);
			vp++;
		}

		if (rem) // move rem leftover bytes to start of buf
			memmove(buf, vp, rem);
	}

	if (rem) {
		memset((char*)buf + rem, ' ', sizeof(SIMD_VEC) - rem);
		SIMD_VEC *vp = buf;
		wcount += count_words(*vp, &state);
		lcount += count_lines(*vp);
	}

	printf(" %7zu %7zu %7zu %s\n", lcount, wcount, ccount, argv[1]);

	close(fd);
	free(buf);
}
