#pragma once

#include <sys/ptrace.h>
#include <sys/user.h>
#include <csignal>

namespace Ptrace {
  void trace_me();
  void continue_exec(pid_t m_pid);
  void single_step(pid_t m_pid);
  void write_memory(uint64_t pid, uint64_t address, uint64_t data);
  uint64_t read_memory(uint64_t pid, uint64_t address);

  void get_registers(uint64_t pid, user_regs_struct* user_regs);
  void set_registers(uint64_t pid, user_regs_struct* user_regs);

  void get_sig_info(uint64_t pid, siginfo_t* info);
}