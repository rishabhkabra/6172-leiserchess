#include <iostream>
#include <cstdio>

int main() {
  std::cout<<"char square_of_table[10][10] = {\n";
  for (int f = 0; f < 10; f++) {
    std::cout<<"{";
    for (int r = 0; r < 10; r++) {
      int a =  (16 * (3 + f) + 3 + r);
      printf(" '\\x%x',", a);
    }
    std::cout<<"}\n";
  }
  std::cout<<"};\n";
  return 0;
}
