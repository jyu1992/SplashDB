#include <iostream>
#include <fstream>
#include <memory>
#include <sstream>

#include "splash_table.hpp"

int main(int argc, char **argv)
{
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " dumpfile\n";
    std::exit(1);
  }

  std::ifstream dumpfile(argv[1]);
  if (!dumpfile.good()) {
    std::cerr << "could not open file \"" << argv[1] << "\"\n";
    std::exit(1);
  }

  std::shared_ptr<const SplashTable> st = SplashTable::fromFile(dumpfile);

  /* read the probe input and print results */
  std::string line;
  uint32_t value;
  while (std::getline(std::cin, line)) {
    std::stringstream(line) >> value;
    std::cout << st->probe(value) << '\n';
  }
}
