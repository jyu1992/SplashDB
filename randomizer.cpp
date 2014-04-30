#include <iostream>
#include <random>
#include <cstdint>

int main(int argc, char **argv)
{
  std::random_device rd;
  std::default_random_engine gen(rd());
  std::uniform_int_distribution<uint32_t> distr;

  uint32_t x;
  while (true) {
    x = distr(gen);
    std::cout << x << ' ' << x << '\n';
  }
}
