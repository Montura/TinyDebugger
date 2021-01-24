#include <iostream>
#include <unistd.h>
#include <sys/personality.h>

#include "debugger.h"
#include "debugger.cpp"
#include "utils.h"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Program name not specified";
    return -1;
  }

  auto programm = argv[1];

  auto pid = fork();
  if (pid == 0) {
    std::cout << "Hello from debugee, pid " << getpid() << "\n";
    personality(ADDR_NO_RANDOMIZE);

    // ptrace allows us to observe and control the execution of another process by reading registers,
    // reading memory, single stepping and more.
    m_ptrace(PT_TRACE_ME, 0, nullptr, 0);
    execl(programm, programm, nullptr);
  } else if (pid >= 1)  {
    // we're in the parent process execute debugger
    std::cout << "Hello from debugger, pid " << getpid() << ", started debugging process " << pid << '\n';
    Debugger<void*> dbg { programm, pid };
    dbg.run();
  }
  return 0;
}
