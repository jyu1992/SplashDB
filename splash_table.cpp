#include <map>
#include <queue>

#include "splash_table.hpp"

SplashTable::SplashTable(size_t numHashes, size_t numBuckets,
    size_t bucketSize, unsigned int maxReinserts)

  : buckets(numBuckets),
    data(new uint32_t[numBuckets * bucketSize * 2]),
    hashes(numHashes),

    numHashes(numHashes),
    numBuckets(numBuckets),
    bucketSize(bucketSize),
    maxReinserts(maxReinserts),

    /* since these are powers of 2, subtracting 1 yields a bitmask that
     * when ANDed with a value is equivalent to taking the value modulo
     * bucketSize or numBuckets, respectively
     */
    bucketMask(bucketSize - 1u),
    tableMask(numBuckets - 1u),

    gen(rd()),
    distrDouble(0.0, 1.0) /* restrict to the range 0-1.0 */

{
  /* populate the data pointers for the bucket table the data is an array of
   * uint32_t, which is divided into units of length 2 * bucketSize, each
   * representing a bucket. In each unit, the first bucketSize elements are the
   * keys, and the next bucketSize elements are the values.
   */
  uint32_t *tmp = data.get();
  for (std::vector<Bucket>::iterator it = buckets.begin();
       it != buckets.end(); ++it)
  {
    it->keys = tmp;
    it->values = tmp + bucketSize;
    tmp += 2 * bucketSize;
  }

  /* create random numbers for multiplicative hashing */
  for (std::vector<uint32_t>::iterator it = hashes.begin();
       it != hashes.end(); ++it)
  {
    /* create a 32 bit odd number for hashing */
    *it = randomUint32() * 2u + 1u;
  }
}

void SplashTable::dump()
{
}

uint32_t SplashTable::randomUint32()
{
  return distrUint32(gen);
}

/* return a random double on range [0, 1.0) */
double SplashTable::randomDouble()
{
  return distrDouble(gen);
}

/* hashes key with the multiplier in hashes[function] */
size_t SplashTable::hashWith(size_t function, uint32_t key)
{
  /* multiply to a 32 bit value */
  uint32_t hash = key * hashes[function];

  /* interpret this as a decimal fraction of 1 and multiply it by the
   * table size */
  return (size_t) ((uint64_t) hash * numBuckets) >> 32;
}

/* compares 2 values and returns a bitmask of all 0's if they are not equal, or
 * a bitmask of all 1's if they are equal */
uint32_t SplashTable::compareToMask(uint32_t a, uint32_t b)
{
  return (uint32_t) -(a == b);
}

// TODO: should we place keys and values in separate arrays to optimize cache?
void SplashTable::insert(uint32_t key, uint32_t value)
{
  if (probe(key) != 0) {
    throw SplashTable::KeyExistsException();
  }

  std::pair<bool, size_t> bucketSelection;
  size_t bucketId, lastBucketId;
  bool selectionSuccess;

  /* whether we have a bucket to avoid. none on the first insert, but for
   * reinserts, we need to avoid the current bucket */
  bool needAvoid = false;

  uint32_t evictedKey, evictedValue;
  unsigned int reinserts = 0;

  while (true) {
    if (reinserts > maxReinserts) {
      throw SplashTable::MaxReinsertsException();
    }

    bucketSelection = bestBucket(key);
    selectionSuccess = bucketSelection.first;
    bucketId = bucketSelection.second;

    /* if we couldn't find a free bucket, select a random one.
     * if necessary, avoid reusing the bucket we just evicted from*/
    if (!selectionSuccess) {
      bucketId = randomBucket(key, needAvoid, lastBucketId);
    }
    Bucket &tmpBucket = buckets[bucketId];
    size_t newPosition;

    /* no more reinserts necessary */
    if (selectionSuccess) {
      newPosition = (tmpBucket.start + tmpBucket.length) & bucketMask;
      tmpBucket.keys[newPosition] = key;
      tmpBucket.values[newPosition] = value;
      ++tmpBucket.length;
      break;
    }

    /* otherwise, continuing reinserting and do some bookkeeping */
    newPosition = tmpBucket.start;
    evictedKey = tmpBucket.keys[newPosition];
    evictedValue = tmpBucket.values[newPosition];

    tmpBucket.keys[newPosition] = key;
    tmpBucket.values[newPosition] = value;
    tmpBucket.start = (tmpBucket.start + 1) & bucketMask;

    key = evictedKey;
    value = evictedValue;

    ++reinserts;
    needAvoid = true;
    lastBucketId = bucketId;
  }
}

/* given a key, finds the best bucket to place it in (the least loaded).
 *
 * first value is a boolean representing success, second value is the index of
 * the selected bucket.
 *
 * if first value is false, then all buckets are loaded, and the second value
 * is undefined.
 */
std::pair<bool, size_t> SplashTable::bestBucket(uint32_t key)
{
  bool foundFree = false;
  size_t bestBucket;
  size_t minLoad;

  /* try all hashes */
  for (size_t hashId = 0; hashId < numHashes; ++hashId) {
    size_t bucketId = hashWith(hashId, key);
    Bucket &bucket = buckets[bucketId];
    if (foundFree && bucket.length < minLoad) {
      bestBucket = bucketId;
      minLoad = bucket.length;
    } else if (!foundFree && bucket.length < bucketSize) {
      bestBucket = bucketId;
      minLoad = bucket.length;
      foundFree = true;
    }
  }

  return std::make_pair(foundFree, bestBucket);
}

/* given a key, randomly return one of the buckets that it hashes to. this
 * function should only be called after bestBucket() has been called and no
 * unloaded bucket was found.
 *
 * the second parameter is a boolean describing whether we need to avoid a
 * particular bucket.
 *
 * if parameter 2 is true, the third parameter is the index of the bucket to
 * avoid - this bucket is only returned if there is no alternative
 */
size_t SplashTable::randomBucket(uint32_t key, bool needAvoid,
    size_t avoidBucket = 0)
{
  size_t bucketId, tmpBucketId;
  double tryNum = 1.0;
  bool success = false;

  /* start at try 1 and continue until we run out of valid potential
   * buckets, maintining a result. on try n, replace the result with our
   * current bucket with probability (1/n). this gives a random distribution
   * over the valid buckets */
  for (size_t hashId = 0; hashId < numHashes; ++hashId) {
    tmpBucketId = hashWith(hashId, key);
    if (!needAvoid || tmpBucketId != avoidBucket) {
      success = true;
      if (randomDouble() < 1.0 / tryNum) {
        bucketId = tmpBucketId;
      }
      tryNum += 1.0;
    }
  }

  /* return the undesirable bucket only if necessary */
  return (success) ? bucketId : avoidBucket;
}

uint32_t SplashTable::probe(uint32_t key)
{
  uint32_t result = 0;

  /* look through all possible buckets to find the key */
  for (size_t hashId = 0; hashId < numHashes; ++hashId) {
    Bucket &bucket = buckets[hashWith(hashId, key)];
    for (size_t i = 0; i < bucket.length; ++i) {
      size_t tmpKey = bucket.keys[(bucket.start + i) & bucketMask];
      size_t tmpValue = bucket.values[(bucket.start + i) & bucketMask];

      /* if tmpKey and key are equal, the mask is 111... this sets result
       * (which is previously 0) to tmpValue. if not equal, mask is 000... and
       * this instruction has no effect */
      result |= tmpValue & compareToMask(tmpKey, key);
    }
  }

  return result;
}
