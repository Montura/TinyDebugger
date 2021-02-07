#include <iostream>
#include <unistd.h>

#if __linux__
  #include <sys/personality.h>
#endif

#include "debugger.h"
#include "ptrace_impl.h"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Program name not specified";
    return -1;
  }

  auto programm = argv[1];

// Test a breakpoint setting on some address:
//    1. Use objdump -d <exe> -o dump
//    2. Open dump and find the main function and locate the instruction which you want to set the breakpoint on.
//    3. However, these addresses may not be absolute.
//    4. If the program is compiled as a position independent executable, then they are offsets from the address which
//   the binary is loaded at.
//    5. Due to address space layout randomization this load address will change with each run of the program.
// Solution:
//    To disable address space layout randomization for the programs we launch, and look up the correct load address.
//    So call to personality(ADDR_NO_RANDOMIZE) before we call execute_debugee in the child process (main.cpp:33)
  auto pid = fork();
  if (pid == 0) {
    std::cout << "Hello from debugee, pid " << getpid() << "\n";
#if __linux__
    personality(ADDR_NO_RANDOMIZE);
#endif
    // ptrace allows us to observe and control the execution of another process by reading registers,
    // reading memory, single stepping and more.
    Ptrace::traceMe();
    execl(programm, programm, nullptr);
  } else if (pid >= 1)  {
    // we're in the parent process execute debugger
    std::cout << "Hello from debugger, pid " << getpid() << ", started debugging process " << pid << '\n';
    Debugger dbg { programm, pid };
    dbg.run();
  }
  return 0;
}
