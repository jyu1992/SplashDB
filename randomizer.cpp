#include <iostream>
#include <random>
#include <cstdint>

int main(int argc, char **argv)
{
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " n\n"
              << "Where n is the number of lines to generate\n";
    std::exit(1);
  }

  int n = atoi(argv[1]);
  if (n < 0) {
    std::cerr << "n cannot be less than 0\n";
    std::exit(1);
  }

  std::random_device rd;
  std::default_random_engine gen(rd());
  std::uniform_int_distribution<uint32_t> distr;

  uint32_t x;
  for (int i = 0; i < n; ++i) {
    x = distr(gen);
    std::cout << x << ' ' << x << '\n';
  }
}
