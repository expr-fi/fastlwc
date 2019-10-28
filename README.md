# Beating ``wc`` with less portable C

## Background

As you're here, you've probably [seen](https://chrispenner.ca/posts/wc)
*[those](https://ummaycoc.github.io/wc.apl/)*
[posts](https://futhark-lang.org/blog/2019-10-25-beating-c-with-futhark-on-gpu.html).
This one is going to be a bit more boring, as there's no brief introduction to
a weird language here; it's all just the C you already know (or don't),
and the only question that we're trying to answer is: how much faster can we go
on a specific – in this case, 6 years old – system with a sprinkle of SIMD?

## A word on *words*

As far as I know, there's no definitive standard for what what counts as a
word, as far as ``wc`` is concerned. In particular, GNU ``wc`` differentiates
between non-whitespace control characters and all the other non-whitespace
characters, while some other implementations (including BSD ``wc`` and
therefore our reference) do not. As we're not actually here to
supplant *any* ``wc``, I consider this unimportant, but this explains the
discrepancies you will see with certain inputs. (Also disregarded are *fancy*
things like locales and multibyte characters).

## About the files

``bsd-wc.c`` is the reference implementation we want to match  
``fastlwc.c`` uses POSIX file I/O, thus supports pipes  
``fastlwc-mmap.c`` uses ``mmap`` for I/O for comparison  
``fastlwc-mmap-mt.c`` tries to use all your CPU as well (you can control the
amount of threads with the environment variable OMP_NUM_THREADS; don't expect
to gain much more than increased user time with SMT/HT)

## Enough. Is it any faster?

Well, as always, it depends. If your results match mine, you might see
something like this:

```
$ ls -hs ../data/big256.txt
1.6G ../data/big256.txt
$ export LC_ALL=C LANG=C
$ /usr/bin/time -f "%E" /bin/cat ../data/big256.txt >/dev/null
0:00.17
$ /usr/bin/time -f "%E" /usr/bin/wc ../data/big256.txt 
  32884992  280497920 1661098496 ../data/big256.txt
0:06.55
$ /usr/bin/time -f "%E" ./bsd-wc ../data/big256.txt 
 32884992 280497920 1661098496 ../data/big256.txt
0:04.18
$ /usr/bin/time -f "%E" ./fastlwc ../data/big256.txt 
 32884992 280497920 1661098496 ../data/big256.txt
0:00.29
$ /usr/bin/time -f "%E" ./fastlwc-mmap ../data/big256.txt 
 32884992 280497920 1661098496 ../data/big256.txt
0:00.29
$ /usr/bin/time -f "%E" ./fastlwc-mmap-mt ../data/big256.txt 
 32884992 280497920 1661098496 ../data/big256.txt
0:00.13
```
