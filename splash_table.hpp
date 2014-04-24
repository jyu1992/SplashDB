#ifndef __SPLASH_TABLE_HPP__
#define __SPLASH_TABLE_HPP__

#include <cstdint>
#include <vector>
#include <memory>
#include <random>
#include <utility>

struct BucketEntry {
  uint32_t key, value;
};

/* stored as a circular array using (start, length)
 * allows easy removal of items from the beginning during reinsert
 */
struct Bucket {
  size_t start, length;
  BucketEntry *data;
};

class SplashTable {
private:
  std::vector<Bucket> buckets;
  std::unique_ptr<BucketEntry[]> data;
  std::vector<uint32_t> hashes;

  const size_t numHashes, numBuckets, bucketSize;
  const unsigned int maxReinserts;

  const size_t bucketMask, tableMask;

  std::random_device rd;
  std::default_random_engine gen;
  std::uniform_int_distribution<uint32_t> distrUint32;
  std::uniform_real_distribution<double> distrDouble;
  uint32_t randomUint32();
  double randomDouble();

  size_t hashWith(size_t function, uint32_t key);
  uint32_t compareToMask(uint32_t a, uint32_t b);
  std::pair<bool, size_t> bestBucket(uint32_t key);
  size_t randomBucket(uint32_t key, bool needAvoid, size_t avoidBucket);

public:
  /* numHashes: number of hash functions
   * numBuckets: number of buckets (must be power of 2)
   * bucketSize: number of entries per bucket (must be power of 2)
   * maxReinserts: how many times to try reinserting
   */
  SplashTable(size_t numHashes, size_t numBuckets, size_t bucketSize,
      unsigned int maxReinserts);

  void insert(uint32_t key, uint32_t value);
  uint32_t probe(uint32_t key);
  void dump();

  class MaxReinsertsException : public std::exception {
    virtual const char *what() const throw()
    {
      return "Max reinserts reached";
    }
  };

  class KeyExistsException : public std::exception {
    virtual const char *what() const throw()
    {
      return "Key exists";
    }
  };
};

#endif
