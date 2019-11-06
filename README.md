# Beating ``wc`` with C

## Background

As you're here, you've probably [seen](https://chrispenner.ca/posts/wc)
[those](https://ummaycoc.github.io/wc.apl/)
[posts](https://futhark-lang.org/blog/2019-10-25-beating-c-with-futhark-on-gpu.html).
This one is going to be a bit more boring, as there's no brief introduction to
a weird language here; it's all just the C you already know (or don't),
and the algorithm is just as simple as the specification (more on that below).
Still, intrigued by the entries above, we'll try to find out exactly how
fast is fast while targeting to replicate their behaviour.

## A word on words, or the differences between actual implementations

A word, as far as *wc* is concerned, is a non-zero-length string of characters, 
delimited by white space. Historically, some implementations used only spaces, 
tabs and newlines as word separators – a straightforward approach, which can 
still be witnessed in simple cases of text tokenisation. POSIX.1, however, 
recommends the use of an equivalent of *isspace*() from the C standard library.

All contemporary implementations also support multi-byte characters, including 
multibyte whitespace characters (as reported by *iswspace*() for the currently 
chosen locale). However, some of them (including BSD derivatives) actually only 
do so when compelled with the **-m** option, the lack of which they take as a 
free pass to disregard localisation altogether, even though you could argue 
that the option should only control whether *wc* should print file lengths in 
characters instead of bytes. Notably, GNU *wc* behaves differently, and decodes 
all the inputs per the current locale regardless of the presence of that option 
to correctly detect all the multibyte word separators in the current locale. 
The performance implications of this difference are quite significant, but also 
easy to draw wrong conclusions from in isolation, as you can see below.

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

The behaviour of *iswspace*() from the C standard library might surprise some, 
as it does not report non-breaking whitespace characters as spaces, unlike many 
similar functions in other languages. GNU *wc* chooses to treat those 
characters as word separators, anyway. GNU *wc* also disregards non-printable 
non-whitespace characters from affecting the word count (i.e. a string of 
control characters surrounded by whitespace is not counted as a word). Finally, 
rounding up this non-exhaustive list of differences are the varying approaches 
to invalid multibyte sequences. Namely, some implementations count all 
undecodable bytes as characters, while for example GNU *wc* does not – it does
count them as bytes, though.

Our application is far simpler; it will (only) behave like a BSD *wc* without 
any options, and thus without the **-m** option; it doesn't even support any 
options! (Or multiple file names for that matter, because it's not a 
replacement for your *wc*!) To have a fairer comparison, the non-multibyte 
POSIX locale is used for all the performance tests. As you can see below, the 
difference is also quite significant (for GNU *wc* in general, or a BSD *wc* 
with **-m**).

```
$ LC_CTYPE=en_US.UTF-8 time_real /usr/bin/wc sample.txt
real 8.92
$ LC_CTYPE=POSIX time_real /usr/bin/wc sample.txt
real 3.96
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
$ export LC_ALL=POSIX
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
