//#include <iostream>

struct test {
  int i;
  float j;
  int k[42];
  test* next;
};

int main() {
  test t;
  int i = t.i;
  float j = t.j;
  int* k = t.k;
  test* next = t.next;
//  std::cout << "i = " << i << "\n";
//  std::cout << "j = " << j << "\n";
//  std::cout << "k = " << k << "\n";
//  std::cout << "next = " << (void*)next << "\n";
//
//  std::cout << "i = " << i << "\n";
//  std::cout << "j = " << j << "\n";
//  std::cout << "k = " << k << "\n";
//  std::cout << "next = " << (void*)next << "\n";

  return 0;
}
