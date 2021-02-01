void a() {
  int foo = 1;
}

void b() {
  int foo = 2;
  a();
}

void c() {
  int foo = 3;
  b();
}

void d() {
  int foo = 4;
  c();
}

void e() {
  int foo = 5;
  d();
}

void f() {
  int foo = 6;
  e();
}

// g++ -g test.cpp -o test
// objdump -d test > test.dump
int main() {
  f();
}