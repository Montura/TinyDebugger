#include <iostream>
#include <unistd.h>
#include <sys/ptrace.h>
#include "debugger.h"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Program name not specified";
    return -1;
  }

  auto programm = argv[1];

  auto pid = fork();
  if (pid == 0) {
    // we're in the child process execute debugee

    // We want to replace whatever we’re currently executing with the program we want to debug.

    // ptrace allows us to observe and control the execution of another process by reading registers, reading memory, single stepping and more.
    int rc = ptrace(PT_TRACE_ME, 0, nullptr, 0);
    if (rc) {
      std::cerr << "Got an error after ptrace(PT_TRACE_ME, 0, nullptr, 0)";
      return -1;
    }
    execl(programm, programm, nullptr);
  } else if (pid >= 1)  {
    // we're in the parent process execute debugger
    std::cout << "Started debugging process " << pid << '\n';
    Debugger dbg { programm, pid };
    dbg.run();
  }
  return 0;
}
