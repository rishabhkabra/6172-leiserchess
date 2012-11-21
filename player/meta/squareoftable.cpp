#include <iostream>
#include <cstdio>

int main() {
  std::cout<<"unsigned char fil_of_table[144] = {\n";
  for (int i = 0; i < 144; i++) {
    std::cout<<(i/12)<<", ";
    if (i % 12 == 11) std::cout<<"\n";
  }
  std::cout<<"};\n";

  std::cout<<"\nunsigned char rnk_of_table[144] = {\n";
  for (int i = 0; i < 144; i++) {
    std::cout<<(i%12)<<", ";
    if (i % 12 == 11) std::cout<<"\n";
  }
  std::cout<<"};\n";

  return 0;
}
