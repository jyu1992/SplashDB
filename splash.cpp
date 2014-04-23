#include <iostream>

#include "splash_table.hpp"

int main()
{
  SplashTable st(4, 32, 1, 8);
  st.put(3, 4);
  st.put(4, 5);
  std::cout << st.get(3) << '\n' << st.get(4) << '\n' << st.get(5) << std::endl;
  return 0;
}
