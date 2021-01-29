#include <cstring>
#include <iostream>

#include "ptrace_impl.h"

#if __linux__
uint64_t m_ptrace(__ptrace_request request, pid_t m_pid, uint64_t addr, uint64_t* data) {
  uint64_t res = ptrace(request, m_pid, addr, data);
  if (res == -1) {
    std::cerr << "Oh dear, something went wrong with" << __PRETTY_FUNCTION__
              << ", error =  " << strerror(errno)
              << ", request = " << request
              << ", pid = " << m_pid
              << ", addr = " << reinterpret_cast<void*>(addr) << "\n";
    exit(res);
  } else {
//      std::cout << "Good with ptrace, res = " << res << "\n";
  }
  return res;
}

void Ptrace::trace_me() {
  m_ptrace(PT_TRACE_ME, 0, 0, nullptr);
}

void Ptrace::continue_exec(pid_t m_pid) {
  m_ptrace(PT_CONTINUE, m_pid, 0, nullptr);
}

uint64_t Ptrace::read_memory(uint64_t pid, uint64_t address) {
  return m_ptrace(PTRACE_PEEKDATA, pid, address, nullptr);
}

void Ptrace::write_memory(uint64_t pid, uint64_t address, uint64_t data) {
  m_ptrace(PTRACE_POKEDATA, pid, address, reinterpret_cast<uint64_t*>(data));
}

void Ptrace::single_step(pid_t m_pid) {
  m_ptrace(PTRACE_SINGLESTEP, m_pid, 0, nullptr);
}

void Ptrace::get_registers(uint64_t pid, user_regs_struct* user_regs) {
  m_ptrace(PTRACE_GETREGS, pid, 0, reinterpret_cast<uint64_t*>(user_regs));
}

void Ptrace::set_registers(uint64_t pid, user_regs_struct* user_regs) {
  m_ptrace(PTRACE_SETREGS, pid, 0, reinterpret_cast<uint64_t*>(user_regs));
}

void Ptrace::get_sig_info(uint64_t pid, siginfo_t* info) {
  ptrace(PTRACE_GETSIGINFO, pid, nullptr, reinterpret_cast<uint64_t*>(info));
}

#elif __APPLE__
  long m_ptrace(int request, pid_t m_pid, uint64_t addr, uint64_t data) {
    int res = ptrace(request, m_pid, reinterpret_cast<caddr_t>(addr), data);
    if (res == -1) {
      std::cerr << "Oh dear, something went wrong with" << __PRETTY_FUNCTION__
                << ", error =  " << strerror(errno)
                << ", request = " << request
                << ", pid = " << m_pid
                << ", addr = " << addr << "\n";
      exit(res);
    } else {
      std::cout << "Good with ptrace, res = " << res << "\n";
    }
    return res;
  }

#endif