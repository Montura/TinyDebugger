#pragma once

#include <sys/ptrace.h>
#include <sys/user.h>
#include <csignal>

#if __APPLE__
  #define PTRACE_PEEKDATA PT_READ_D
  #define PTRACE_POKEDATA PT_WRITE_D
  #define PTRACE_SINGLESTEP PT_STEP
#endif

namespace Ptrace {
  void traceMe();
  void continueExec(pid_t m_pid);
  void singleStep(pid_t m_pid);
  void writeMemory(uint64_t pid, uint64_t address, uint64_t data);
  uint64_t readMemory(uint64_t pid, uint64_t address);

  void getRegisters(uint64_t pid, user_regs_struct* user_regs);
  void setRegisters(uint64_t pid, user_regs_struct* user_regs);

  void getSigInfo(uint64_t pid, siginfo_t* info);
}