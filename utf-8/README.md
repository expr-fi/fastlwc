## Regarding UTF-8

Nothing about multibyte characters (or UTF-8 in particular) prevent
benefitting from the use of SIMD instructions. Also, there's nothing novel
about validating data as UTF-8 using SIMD, as the benefits are obvious with
larger inputs.

But perhaps no validation is required here, anyway. When it comes to invalid
multibyte sequences, the only common behaviour between existing ``wc``
implementations is that they do not fail (at most they warn about it), and
produce some deterministic output. This program's deterministic output is
somewhere between that of GNU and Apple with non-UTF-8 inputs. With valid
UTF-8 it behaves just like Apple ``wc`` with options **-lmw**, only somewhat
faster on my machine.

```
$ ls -hs /tmp/enwikisource-20191020-pages-articles-multistream.xml
9.1G /tmp/enwikisource-20191020-pages-articles-multistream.xml
$ time -p bin/fastlwc-utf8 /tmp/enwikisource-20191020-pages-articles-multistream.xml
 157190631 1331818642 9680393127 /tmp/enwikisource-20191020-pages-articles-multistream.xml
real 1.64
user 0.66
sys 0.98
$ time -p bin/bsd-wc -lmw /tmp/enwikisource-20191020-pages-articles-multistream.xml
 157190631 1331818642 9680393127 /tmp/enwikisource-20191020-pages-articles-multistream.xml
real 206.95
user 204.51
sys 2.44
```
