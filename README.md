SplashDB: in-memory key-value store
===================================

Building
--------
To build, simply type 'make'. The Makefile is configured to use gcc and g++ by
default; however, clang and clang++ have been tested working as well. This
program requires compiler support for SSE intrinsics and CPU support for the
SSE instructions themselves.

The build produces three binaries: randomizer, splash, and probe. randomizer
and splash and implemented in C++, and probe is implemented in C.

Usage
-----
```bash
./randomizer n
```

Produces n 32-bit unsigned integers and dumps to stdout. These can be used as
keys or values for testing as follows:
```bash
./randomizer 1000 > keys
./randomizer 1000 > values
paste -d' ' keys values > input
```

Note that there is a possiblity of duplicates, which obviously increases as n
becomes large. This may pose a problem for testing.

To run splash, use:
```bash
./splash B R S h input dump < keys > output
```

where B is the bucket size, R is the number of reinsertions to attempt, S is
the log base 2 of the table capacity, input is the name of an input file, keys
is the name of a probe file, and output is the name of the result file. All of
these follow the format given in the assignment. The optional parameter dump is
the name of a dump file; if specified, a dump will be performed (format follows
the assignment specifications).

To run probe, use:
```bash
./probe dump < keys > output
```

where dump is a dump file produced by splash, keys is a probe file, and output
is the name of the result file.

Error checking
--------------
The programs have some basic level of error checking, but in general it is
assumed that the input files follow the specifications. The splash command will
check that the table parameters are valid, but does not necessarily check that
they are reasonable.

Implementation
--------------
Both probe and splash organize their data in the same way. There is an array of
uint32_t of size 2 * numBuckets = 2 * 2^S / B. This array is divided into
buckets, where given bucket size B, we have B keys, followed by their
corresponding B values, end to end until the end of the array. There is also an
array of Bucket structures, which serves primarily to keep pointers into the
key and value locations of each bucket.

If insertion requires an eviction, the least loaded bucket is selected. If all
buckets are loaded, we randomly select one. We only reinsert into the same
bucket if absolutely necessary.

The probe algorithm makes use of a comparison bitmask, which performs a
comparison and yields 0xffff... if the values are equal and 0x0 otherwise. For
splash, we simply iterate over all bucket slots, ANDing the value with the
comparison bitmask of the key and the lookup key, and ORing all of those
results together.

The vectorized version essentially does the same, but with SIMD instructions
rather than loops.
