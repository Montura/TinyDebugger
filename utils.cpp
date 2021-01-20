#include <iostream>

#include "utils.h"

bool is_prefix(const std::string& s, const std::string& of) {
  if (s.size() > of.size()) {
    return false;
  }
  return !s.empty() && std::equal(s.begin(), s.end(), of.begin());
}

int m_ptrace(int request, pid_t m_pid, caddr_t addr, int data) {
  int hr = ptrace(request, m_pid, addr, data);
  if (hr == -1) {
    std::cerr << "Oh dear, something went wrong with" << __PRETTY_FUNCTION__
              << ", error =  " << strerror(errno)
              << ", request = " << request
              << ", pid = " << m_pid
              << ", addr = " << addr << "\n";
    return hr;
  }
  return 0;
}

