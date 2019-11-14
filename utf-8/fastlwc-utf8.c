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

#include "simd-ext.h"

#define BUFSIZE (sizeof(simd_vector) * 4096)

// I see potential for improvement here
typedef lcount_state ncount_state;
#define NCOUNT_INITIAL LCOUNT_INITIAL

// count continuation bytes 0b10xxxxxx just like line feeds
static inline uint64_t count_continuations_final(ncount_state *state)
{
	state->count = simd_add_i64(state->count,
	                            simd_sad_u8(state->vcount, simd_setzero()));
	state->vcount = simd_setzero();
	int64_t sum = 0;
	for (int i = 0; i != sizeof(simd_vector)/sizeof(uint64_t); ++i)
		sum += ((uint64_t*)&state->count)[i];
	return sum;
}
static inline int count_continuations(simd_vector vec, ncount_state *state)
{
	simd_vector is_cont = simd_cmpeq_i8(simd_and(vec, simd_set_i8('\xC0')),
	                                    simd_set_i8('\x80'));
	state->vcount = simd_sub_i8(state->vcount, is_cont);
	state->iterations++;
	if (state->iterations == 255) {
		// sum counts before they can overflow
		state->count = simd_add_i64(state->count,
		                            simd_sad_u8(state->vcount, simd_setzero()));
		state->vcount = simd_setzero();
		state->iterations = 0;
	}
	return 0;
}

// pretty stateful, eh
struct count_state
{
	size_t lcount, wcount, ncount;
	lcount_state lstate;
	wcount_state wstate;
	ncount_state nstate;
	simd_vector xprev;
	simd_mask ws_rollover;
	bool prev_mb;
};

static inline void initialise(struct count_state *state)
{
	state->lcount = state->wcount = state->ncount = 0;
	state->lstate = LCOUNT_INITIAL;
	state->wstate = WCOUNT_INITIAL;
	state->nstate = NCOUNT_INITIAL;
	state->xprev = simd_setzero();
	state->ws_rollover = simd_mask_init();
	state->prev_mb = false;
}

static inline void update(simd_vector vec, struct count_state *state)
{
	simd_vector xvec = simd_xor(vec, simd_set_i8('\x80'));
	bool vec_mb = has_multibyte_sequence(vec);

	if (state->prev_mb) {
		// prev contained multibyte sequences, so it wasn't counted for words
		// as there's a possibility some of them are completed by vec
		// rolled over are information on if prev completed multibyte sequence
		// as well as any single-byte whitespace in prev
		simd_mask ws = state->ws_rollover;

		simd_mask mbws = simd_cmpws_xmb_i8_mask(state->xprev, xvec);
		if (simd_mask_none(mbws)) {
			// avoid doing "expensive" expanding if there's nothing to expand
			state->ws_rollover = simd_mask_init();
		} else {
			// mbws only has bits set for the leading byte of multibyte
			// sequence, so those need to be expanded (1 -> 111)
			// also store any rollover for later
			mbws = simd_mask_expandl3(mbws, &state->ws_rollover);
			ws = simd_mask_or(ws, mbws);
		}

		state->wcount += count_words_wsmask(ws, &state->wstate);
	}

	state->lcount += count_lines(vec, &state->lstate);
	simd_mask sbws = simd_cmpws_i8_mask(vec);

	if (!vec_mb) {
		state->prev_mb = false;
		state->wcount += count_words_wsmask(sbws, &state->wstate);
	} else {
		// can't count words before getting the next block
		// as there's no knowing whether vec ends in whitespace or not
		// count continuation bytes instead
		state->prev_mb = true;
		state->xprev = xvec;
		state->ncount += count_continuations(vec, &state->nstate);
		state->ws_rollover = simd_mask_or(state->ws_rollover, sbws);
	}
}

static inline void finalise(struct count_state *state)
{
	// "flush" the previous slice waiting for completion
	if (state->prev_mb)
		update(simd_set_i8(' '), state);

	state->lcount += count_lines_final(&state->lstate);
	state->wcount += count_words_final(&state->wstate);
	state->ncount += count_continuations_final(&state->nstate);
}

int main(int argc, char *argv[])
{
	char *buf = aligned_alloc(sizeof(simd_vector), BUFSIZE);
	if (!buf) {
		perror("fastlwc: alloc");
		exit(EXIT_FAILURE);
	}

	int fd = (argc < 2 || strcmp(argv[1], "-") == 0 ? STDIN_FILENO
	                                                : open(argv[1], O_RDONLY));
	if (fd < 0) {
		perror("fastlwc: open");
		exit(EXIT_FAILURE);
	}

	posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);

	ssize_t len;
	size_t rem = 0,
	       ccount = 0;

	struct count_state state;
	initialise(&state);

	do {
		len = read(fd, buf + rem, BUFSIZE - rem);
		if (len < 0) {
			perror("fastlwc: read");
			exit(EXIT_FAILURE);
		}

		if (len > 0) {
			rem += len;
			ccount += len;
		} else {
			if (!rem)
				break;
			memset(buf + rem, ' ', sizeof(simd_vector) - rem);
			rem = sizeof(simd_vector);
		}

		simd_vector *vp = (simd_vector*)buf;
		while (rem >= sizeof(simd_vector)) {
			update(*vp, &state);

			rem -= sizeof(simd_vector);
			vp++;
		}

		if (rem) // move rem leftover bytes to start of buf
			memmove(buf, vp, rem);
	} while (len);

	finalise(&state);
	size_t mcount = ccount - state.ncount;

	printf(" %7zu %7zu %7zu", state.lcount, state.wcount, mcount);
	if (argc >= 2)
		printf(" %s", argv[1]);
	putchar('\n');

	close(fd);
	free(buf);
}
