#!/bin/sh

invalid_result() {
	set +x
	echo "Invalid result: $1"
	echo "Expected:"
	cat "$tmpdir/ref"
	echo "Got:"
	cat "$tmpdir/cmp"
	exit 1
}

rm_tmpdir() {
	test "$tmpdir/" && rm -r "$tmpdir/"
}

sde_arch() {
	if ! test -x "$SDE"; then	
		if ! which sde >/dev/null; then
			echo "error: sde not found in PATH and SDE environment variable not set"
			exit 1
		else
			SDE=sde
		fi
	fi
	
	for f in "$1"/*; do
		rm -f "$tmpdir/cmp"
		for input in data/*; do
			$SDE -- "$f" "$input" >>"$tmpdir/cmp"
		done
		cmp "$tmpdir/ref" "$tmpdir/cmp" || invalid_result "$f"
	done
}

real_arch() {
	for f in "$1"/*; do
		rm -f "$tmpdir/cmp"
		for input in data/*; do
			"$f" "$input" >>"$tmpdir/cmp"
		done
		cmp "$tmpdir/ref" "$tmpdir/cmp" || invalid_result "$f"
	done
}

trap rm_tmpdir EXIT
set -xe

tmpdir=$(mktemp -d)

for input in data/*; do
	bin/bsd-wc "$input" >>"$tmpdir/ref"
done

for arch; do
	case "$arch" in
		e-*) sde_arch "${arch#e-}";;
		*) real_arch "$arch";;
	esac
done

set +x
echo "All tests passed"
