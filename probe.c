#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include <x86intrin.h>

struct Bucket {
  uint32_t *keys, *values;
};

union QuadInt {
  __m128i vec128;
  __m64 vec64[2];
  uint32_t arr[4];
};

static FILE *dumpfile = NULL;
static char *buffer = NULL;
static struct Bucket *buckets = NULL;
static uint32_t *data = NULL;

static size_t b, s, h, n;
static size_t bucketCount;
static union QuadInt hashes;
static size_t hashShift;

uint32_t probe(uint32_t key)
{
  /* create a vector with all values initialized to key */
  __m128i keyVector = _mm_set1_epi32(key);

  /* find the appropriate buckets using multiplicative hashing */
  __m128i bucketIds = _mm_mullo_epi32(keyVector, hashes.vec128);
  bucketIds  = _mm_srli_epi32(bucketIds, hashShift);
  size_t b0 = _mm_extract_epi32(bucketIds, 0);
  size_t b1 = _mm_extract_epi32(bucketIds, 1);

  __m128i keys;
  __m128i values0, values1;

  /* load keys, compare with lookup key (to produce a bitmask).
   * AND the result with the corresponding values. */
  keys = _mm_load_si128((const __m128i *) buckets[b0].keys);
  keys = _mm_cmpeq_epi32(keys, keyVector);
  values0 = _mm_load_si128((const __m128i *) buckets[b0].values);
  values0 = _mm_and_si128(values0, keys);

  keys = _mm_load_si128((const __m128i *) buckets[b1].keys);
  keys = _mm_cmpeq_epi32(keys, keyVector);
  values1 = _mm_load_si128((const __m128i *) buckets[b1].values);
  values1 = _mm_and_si128(values1, keys);

  /* OR all of the (key AND value) pairs to get result */
  union QuadInt qi;
  qi.vec128 = _mm_or_si128(values0, values1);
  qi.vec64[0] = _mm_or_si64(qi.vec64[0], qi.vec64[1]);
  return qi.arr[0] | qi.arr[1];
}

int main(int argc, char **argv)
{
  if (argc < 2) {
    fprintf(stderr, "Usage: %s dumpfile\n", argv[0]);
    goto OUT;
  }

  buffer = malloc(4096);
  if (!buffer) {
    perror("exiting");
    goto OUT;
  }

  /* open dumpfile */
  dumpfile = fopen(argv[1], "r");
  if (!dumpfile) {
    perror(argv[1]);
    goto OUT;
  }

  /* read table parameters */
  if (!fgets(buffer, 4096, dumpfile) ||
      sscanf(buffer, "%zd %zd %zd %zd", &b, &s, &h, &n) != 4)
  {
    fprintf(stderr, "Invalid dumpfile\n");
    goto OUT;
  }
  bucketCount = (1u << s) / b;
  hashShift = __builtin_clz(bucketCount - 1u);

  /* read hashes */
  if (!fgets(buffer, 4096, dumpfile) ||
      sscanf(buffer, "%" SCNu32 " %" SCNu32,
        &hashes.arr[0], &hashes.arr[1]) != 2)
  {
    fprintf(stderr, "Invalid dumpfile\n");
    goto OUT;
  }

  /* allocate memory for table */
  buckets = calloc(bucketCount, sizeof(*buckets));
  data = calloc((1u << s) * 2, sizeof(*data));;
  if (!buckets || !data) {
    perror("exiting");
    goto OUT;
  }

  /* set up bucket pointers and populate table */
  uint32_t *ptr = data;
  uint32_t key, value;
  for (int i = 0; i < bucketCount; ++i) {
    buckets[i].keys = ptr;
    buckets[i].values = ptr + b;
    ptr += 2 * b;

    for (int j = 0; j < b; ++j) {
      if (!fgets(buffer, 4096, dumpfile) ||
          sscanf(buffer, "%" SCNu32 " %" SCNu32, &key, &value) != 2)
      {
        fprintf(stderr, "Invalid dumpfile\n");
        goto OUT;
      }
      buckets[i].keys[j] = key;
      buckets[i].values[j] = value;
    }
  }

  /* probe */
  while (fgets(buffer, 4096, stdin)) {
    if (sscanf(buffer, "%" SCNu32, &key) != 1) {
      continue;
    }

    value = probe(key);
    if (value != 0) {
      fprintf(stdout, "%" SCNu32 " %" SCNu32 "\n", key, value);
    }
  }

OUT:
  if (dumpfile) fclose(dumpfile);
  free(buffer);
  free(buckets);
  free(data);

  return 0;
}
