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

## A word on words

A word, as far as *wc* is concerned, is a non-zero-length string of characters,
delimited by white space. Among widely used implementations, this succinct
definition has resulted in slight differences when it comes to certain inputs. 
Specifically, GNU *wc* doesn't count strings of non-printable non-white space
characters as words, while some other implementations, including some BSD
derivatives, do.
As the purpose of this demo is not to supplant any *wc*, I consider this matter
unimportant, but nevertheless our reference is the latter. Also out of scope
are support for locales other than POSIX (i.e. supports ASCII white space only,
and with that limitation, ASCII-compatible inputs, e.g. UTF-8), in which the
comparisons below are run.

## About the files

``fastlwc.c`` uses file I/O, can read from *stdin*  
``fastlwc-mmap.c`` uses ``mmap`` for I/O for comparison  
``fastlwc-mmap-mt.c`` tries to use all your CPU as well (you can control the
amount of threads with the environment variable OMP_NUM_THREADS; don't expect
to gain much more than increased user time with SMT/Hyper-threading)  
``bsd-wc.c`` is the reference implementation we test against (it's a simplified
fast path from Apple's *wc*)  

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
real 6.54
user 6.36
sys 0.18
$ time -p bin/bsd-wc big256.txt 
 32884992 280497920 1661098496 big256.txt
real 4.11
user 3.74
sys 0.36
$ time -p bin/fastlwc big256.txt 
 32884992 280497920 1661098496 big256.txt
real 0.23
user 0.06
sys 0.17
$ time -p bin/fastlwc-mmap-mt big256.txt 
 32884992 280497920 1661098496 big256.txt
real 0.12
user 0.18
sys 0.14
```
