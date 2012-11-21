#include <iostream>
#include <cstdio>

int main() {
  std::cout<<"unsigned char fil_of_table[144] = {\n\t";
  for (int i = 0; i < 144; i++) {
    int f;
    if (i < 12 || i >= 132 || (i % 12 == 0) || (i % 12 == 11)) {
      f = 10;
    } else {
      f = i/12 - 1;
    }
    printf("'\\x%x', ", f);
    if (i % 12 == 11) std::cout<<"\n\t";
  }
  std::cout<<"};\n";

  std::cout<<"\nunsigned char rnk_of_table[144] = {\n\t";
  for (int i = 0; i < 144; i++) {
    int r;
    if (i < 12 || i >= 132 || (i % 12 == 0) || (i % 12 == 11)) {
      r = 10;
    } else {
      r = i%12 - 1;
    }
    printf("'\\x%x', ", r);
    if (i % 12 == 11) std::cout<<"\n\t";
  }
  std::cout<<"};\n";

  return 0;
}
