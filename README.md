# Beating ``wc`` with C

## Background

As you're here, you've probably [seen][p1] [those][p2] [posts][p3].
Arguably, this one might be a bit more boring, as there's no brief
introduction to a weird language here; it's all just the C you already know
(or don't), and the algorithm used is just as simple as the task at hand
(more on that below). Still, intrigued by the entries mentioned above, the
goal here is to find out exactly how fast is fast when it comes to ``wc``.

[p1]: https://chrispenner.ca/posts/wc
[p2]: https://ummaycoc.github.io/wc.apl/
[p3]: https://futhark-lang.org/blog/2019-10-25-beating-c-with-futhark-on-gpu.html

## A word on words, or the differences between actual ``wc`` implementations

A word, as far as ``wc`` is concerned, is a non-zero-length string of
characters, delimited by white space ([POSIX.1-2017]). As [some][netbsd]
[implementations][openbsd] have noted, no distinction is made between
printable and non-printable characters, yet for example GNU ``wc`` ignores
non-printable (non-whitespace) characters when counting words. Historical
implementations regarded only spaces, tabs and newlines as whitespace – a
straightforward approach, which can still be seen in use today. POSIX.1,
however, recommends the use of an equivalent of *isspace()* from the C
standard library.

All non-historical implementations support multibyte characters, including
multibyte whitespace characters (when using a multibyte locale). However,
[some][apple] [of][freebsd] [them][openbsd] only do so when compelled with
the **-m** option, which is used to print input lengths in characters
instead of bytes. [Other][netbsd] [implementations][GNU] recognise multibyte
whitespace characters when counting words regardless of that option. This
not only alters the behaviour, but has also considerable performance
implications when ``wc`` is not given any options, i.e. the default use
case, as highlighted below. (This disparity has been mistakenly attributed
to different reasons in previous discussions.)

```
$ export LC_CTYPE=en_US.UTF-8
$ time_real bin/bsd-wc sample.txt
real 4.70
$ time_real /usr/bin/wc sample.txt
real 8.92
$ time_real bin/bsd-wc -lmw sample.txt
real 20.53
$ time_real /usr/bin/wc -lmw sample.txt
real 8.93
```

Further, the behaviour of *iswspace()* from the C standard library might
surprise some, as it does not report non-breaking spaces as spaces, unlike
many similar functions in other languages, or what intuition might perhaps
suggest. GNU ``wc`` chooses to treat those characters as word separators,
anyway.

Finally, rounding up this non-exhaustive list are the varying approaches to
invalid multibyte sequences (when they are decoded at all, as touched
earlier). Namely, some implementations count all undecodable bytes as
non-whitespace characters, for both character and word counting, while others
ignore them likewise.

All that being said, this application “averts” these issues by being far
simpler; it doesn't support the **-m** option (in fact, it doesn't support
any options at all, because it's not a replacement for your ``wc``!) or
multibyte characters in general; in other words, its target is to behave
just like Apple's ``wc`` when given no options. For a fairer performance
comparison, the non-multibyte POSIX locale is used for the tests at the end.
As you can see below, that, too, has a significant effect on performance.

```
$ LC_CTYPE=POSIX time_real /usr/bin/wc sample.txt
real 3.96
$ LC_CTYPE=en_US.UTF-8 time_real /usr/bin/wc sample.txt
real 8.92
```

[POSIX.1-2017]: https://pubs.opengroup.org/onlinepubs/9699919799/utilities/wc.html
[netbsd]: http://cvsweb.netbsd.org/bsdweb.cgi/src/usr.bin/wc/wc.c?rev=1.35
[openbsd]: https://cvsweb.openbsd.org/src/usr.bin/wc/wc.c?rev=1.26
[freebsd]: https://svnweb.freebsd.org/base/head/usr.bin/wc/wc.c?revision=346316
[apple]: https://opensource.apple.com/source/text_cmds/text_cmds-99/wc/wc.c
[gnu]: http://git.savannah.gnu.org/cgit/coreutils.git/plain/src/wc.c

## About the algorithm

As the saying goes about some software patents reducing basically to “doing
X, with a computer”, what's going on here is little more than “doing X, with
SIMD”. Word counting in ``wc`` is typically done by looping over the input
characters, one by one, keeping state of the whitespace-ness of the previous
character to know when to increase the counter. This process can be
expressed as a couple of binary operations, by first mapping the input as
whitespace (0) and non-whitespace (1). Shifting that binary value by one
digit and inverting it produces a mask that can be used to find word
boundaries, which computers are great at counting.

```
Just a sample (multiple   spaces).
1111010111111011111111100011111111             =[1]
0111101011111101111111110001111111 SHIFT [1]   =[2]
1000010100000010000000001110000000 NOT [2]     =[3]
1000010100000010000000000010000000 [1] AND [3]
```

## About the files

``fastlwc.c`` uses file I/O, can read from *stdin*  
``fastlwc-mmap.c`` uses ``mmap`` for I/O for comparison  
``fastlwc-mmap-mt.c`` tries to use all your CPU as well (you can control the
amount of threads with the environment variable OMP_NUM_THREADS; don't expect
to gain much more than increased user time with SMT/Hyper-threading)  
``bsd-wc.c`` is the reference implementation we test against (it's basically
*wc* from opensource.apple.com)  

## Enough. Is it any faster?

Well, as always, it depends. If your results match mine, you might see
something like this:

```
$ ls -hs big256.txt
1.6G big256.txt
$ export LC_CTYPE=POSIX
$ time -p /bin/cat big256.txt >/dev/null
real 0.17
user 0.00
sys 0.17
$ time -p /usr/bin/wc big256.txt
  32884992  280497920 1661098496 big256.txt
real 6.55
user 6.29
sys 0.26
$ time -p bin/bsd-wc big256.txt
 32884992 280497920 1661098496 big256.txt
real 7.79
user 7.41
sys 0.36
$ time -p bin/fastlwc big256.txt
 32884992 280497920 1661098496 big256.txt
real 0.22
user 0.06
sys 0.16
$ time -p bin/fastlwc-mmap-mt big256.txt
 32884992 280497920 1661098496 big256.txt
real 0.12
user 0.18
sys 0.14
```

For additional remarks, see the [utf-8 directory](utf-8/).
