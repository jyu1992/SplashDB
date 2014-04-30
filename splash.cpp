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

  if ((unsigned) s > sizeof(uint32_t) * 8) {
    std::cerr << "S is too large" << std::endl;
    std::exit(1);
  }

  if (h < 1 || h > 100) {
    std::cerr << "h must be from 1-100" << std::endl;
    std::exit(1);
  }

  /* read the input file and build the table */
  std::ifstream input(argv[5]);
  if (!input.good()) {
    std::cerr << "could not open file \"" << argv[5] << "\"\n";
    std::exit(1);
  }

  bool shouldProbe = true;;

  SplashTable st(h, (1u << s) / b, b, r);
  try {
    st.build(input);
  } catch (SplashTable::MaxReinsertsException) {
    std::cerr << "Notice: max reinserts reached, table build "
      "terminating early\n";
    std::cerr << "Total of " << st.size() << " successful inserts\n";
    shouldProbe = false;
  } catch (SplashTable::KeyExistsException) {
    std::cerr << "Notice: attempted to insert duplicate key, table "
      "build terminating early\n";
    std::cerr << "Total of " << st.size() << " successful inserts\n";
    shouldProbe = false;
  }

  /* read the probe input and print results */
  if (shouldProbe) {
    std::string line;
    uint32_t key, value;
    while (std::getline(std::cin, line)) {
      std::stringstream(line) >> key;
      value = st.probe(key);
      if (value != 0) {
        std::cout << key << ' ' << value << '\n';
      }
    }
  }

  if (argc > 6) {
    std::ofstream dumpFile(argv[6]);
    if (!dumpFile.good()) {
      std::cerr << "dump file \"" << argv[6]
                << "\" could not be opened for writing\n";
    }
    st.dump(dumpFile);
  }
}
