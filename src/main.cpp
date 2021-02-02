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
