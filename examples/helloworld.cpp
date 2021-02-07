#include <iostream>

// 1. Start debugging the program
// 2. Note what the child process id is
// 3. Read /proc/<pid>/maps to find the load address
//    555555554000-555555555000 r-xp 00000000 08:06 1608698   tinyDebugger/helloworld
// 4. Add an offset from helloworld.dump to the local address:
//   helloworld.dump:146 -> offset = 87e, base = 0x555555554000 =>
//   break 0x55555555487e halt process before printing "HelloWorld!"
//   break 0x555555554883 halt process after printing "HelloWorld!"
int main() {
  std::cerr << (void*)(&main) << "\n";
  std::cerr << "HelloWorld!\n";
  return 0;
}