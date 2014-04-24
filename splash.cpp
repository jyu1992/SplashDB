#include <iostream>
#include <fstream>
#include <sstream>

#include <string>
#include <cstdlib>

#include "splash_table.hpp"

int main(int argc, char **argv)
{
  if (argc < 6) {
    std::cerr << "Usage: " << argv[0] << " B R S h inputfile [dumpfile]\n"
              << "B: bucket size (power of 2)\n"
              << "R: maximum reinsertions\n"
              << "S: base 2 log of capacity\n"
              << "h: number of hash functions" << std::endl;
    std::exit(1);
  }

  int b = atoi(argv[1]);
  int r = atoi(argv[2]);
  int s = atoi(argv[3]);
  int h = atoi(argv[4]);

  if (b < 1 || b & (b-1)) {
    std::cerr << "B must be a power of 2, at least 1" << std::endl;
    std::exit(1);
  }

  if (r < 0) {
    std::cerr << "R cannot be negative" << std::endl;
    std::exit(1);
  }

  if (s < 0) {
    std::cerr << "S cannot be negative" << std::endl;
    std::exit(1);
  }

  if ((unsigned) s > sizeof(size_t) * 8 - 1) {
    std::cerr << "S is too large" << std::endl;
    std::exit(1);
  }

  if (h < 1 || h > 100) {
    std::cerr << "h must be from 1-100" << std::endl;
    std::exit(1);
  }

  std::ifstream input(argv[5]);
  if (!input.good()) {
    std::cerr << "input file \"" << argv[5] << "\" not found" <<std::endl;
    std::exit(1);
  }

  SplashTable st(h, (1u << s) / b, b, r);

  std::string line;
  uint32_t key, value;
  while (std::getline(input, line)) {
    std::stringstream(line) >> key >> value;
    st.insert(key, value);
  }
}
