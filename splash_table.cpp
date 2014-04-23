#include <map>
#include <queue>

#include "splash_table.hpp"

SplashTable::SplashTable(size_t numHashes, size_t numBuckets,
    size_t bucketSize, unsigned int maxReinserts)

  : buckets(numBuckets),
    data(new BucketEntry[numBuckets * bucketSize]),
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

    gen(rd())

{
  /* populate the data pointers for the bucket table */
  BucketEntry *be_tmp = data.get();
  for (std::vector<Bucket>::iterator it = buckets.begin();
       it != buckets.end(); ++it)
  {
    it->data = be_tmp;
    be_tmp += bucketSize;
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

// TODO: this function performs a TON of unnecessary copy operations
// TODO: should we place keys and values in separate arrays to optimize cache?
void SplashTable::put(uint32_t key, uint32_t value)
{
  if (get(key) != 0) {
    throw SplashTable::KeyExistsException();
  }

  /* keep a working copy of the elements of the buckets we are reinserting
   * into */
  std::map<size_t, std::queue<BucketEntry>> workingCopy;

  std::pair<size_t, bool> bucketSelection;
  size_t bucketId;
  bool needsReinsert;
  uint32_t currentKey = key;
  uint32_t currentValue = value;
  unsigned int reinserts = 0;

  while (true) {
    if (reinserts > maxReinserts) {
      throw SplashTable::MaxReinsertsException();
    }

    bucketSelection = bestBucket(currentKey);
    bucketId = bucketSelection.first;
    needsReinsert = bucketSelection.second;

    Bucket &tmpBucket = buckets[bucketId];

    if (workingCopy.count(bucketId) == 0) {
      workingCopy[bucketId] = std::queue<BucketEntry>();
      for (size_t i = 0; i < tmpBucket.length; ++i) {
        workingCopy[bucketId].push(
            tmpBucket.data[(tmpBucket.start + i) & bucketMask]);
      }
    }
    workingCopy[bucketId].push({currentKey, currentValue});

    if (!needsReinsert) {
      break;
    }
    ++reinserts;

    BucketEntry &oldest = workingCopy[bucketId].front();
    currentKey = oldest.key;
    currentValue = oldest.value;
    workingCopy[bucketId].pop();
  }

  /* commit the working copy to the main store */
  for (std::map<size_t, std::queue<BucketEntry>>::iterator it = workingCopy.begin();
       it != workingCopy.end(); ++it)
  {
    bucketId = it->first;
    Bucket &tmpBucket = buckets[bucketId];
    std::queue<BucketEntry> &bucketCopy = it->second;

    tmpBucket.length = bucketCopy.size();
    for (size_t i = 0; bucketCopy.size() > 0; ++i) {
      tmpBucket.data[(tmpBucket.start + i) & bucketMask] = bucketCopy.front();
      bucketCopy.pop();
    }
  }
}

/* given a key, finds the best bucket to place it in (the least loaded). if all
 * are loaded, select one at random. returns a pair (x, y) such that x is the
 * bucket index and y is a boolean, describing whether a reinsert is necessary
 */
std::pair<size_t, bool> SplashTable::bestBucket(uint32_t key)
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

  if (foundFree) {
    return std::make_pair(bestBucket, false);
  } else{
    return std::make_pair(hashWith(randomUint32() % numHashes, key),
        true);
  }
}

uint32_t SplashTable::get(uint32_t key)
{
  uint32_t result = 0;

  /* look through all possible buckets to find the key */
  for (size_t hashId = 0; hashId < numHashes; ++hashId) {
    Bucket &bucket = buckets[hashWith(hashId, key)];
    for (size_t i = 0; i < bucket.length; ++i) {
      BucketEntry& entry = bucket.data[(bucket.start + i) & bucketMask];

      /* if entry.key and key are equal, the mask is 111... this sets result
       * (which is previously 0) to to entry.value. if not equal, mask is
       * 000... and this instruction has no effect */
      result |= entry.value & compareToMask(entry.key, key);
    }
  }

  return result;
}
