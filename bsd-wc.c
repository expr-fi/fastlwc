/*
 * Copyright (c) 1980, 1987, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/statfs.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	if (argc != 2) {
		puts("usage: bsd-wc FILE");
		exit(EXIT_FAILURE);
	}

	struct statfs fsb;
	uintmax_t linect, wordct, charct;
	int fd, len;
	short gotsp;
	uint8_t *p;
	uint8_t *buf;

	linect = wordct = charct = 0;

	if ((fd = open(argv[1], O_RDONLY, 0)) < 0) {
		perror("open");
		exit(EXIT_FAILURE);
	}

	if (fstatfs(fd, &fsb)) {
		perror("fstatfs");
		exit(EXIT_FAILURE);
	}

	buf = malloc(fsb.f_bsize);
	if (!buf) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	gotsp = 1;
	while ((len = read(fd, buf, fsb.f_bsize)) != 0) {
		if (len == -1) {
			perror("read");
			exit(EXIT_FAILURE);
		}
		p = buf;
		while (len > 0) {
			uint8_t ch = *p;
			charct++;
			len -= 1;
			p += 1;
			if (ch == '\n')
				++linect;
			if (isspace(ch))
				gotsp = 1;
			else if (gotsp) {
				gotsp = 0;
				++wordct;
			}
		}
	}

	printf(" %7ju", linect);
	printf(" %7ju", wordct);
	printf(" %7ju", charct);

	printf(" %s\n", argv[1]);

	close(fd);
}
