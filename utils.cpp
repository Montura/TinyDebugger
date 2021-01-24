#include <cstring>
#include <iostream>

#include "utils.h"

bool is_prefix(const std::string& s, const std::string& of) {
  if (s.size() > of.size()) {
    return false;
  }
  return !s.empty() && std::equal(s.begin(), s.end(), of.begin());
}

#if __linux__
  long m_ptrace(__ptrace_request request, pid_t m_pid, void* addr, uint64_t data) {
    long res = ptrace(request, m_pid, addr, data);
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
#elif __APPLE__
  int m_ptrace(int request, pid_t m_pid, caddr_t addr, int data) {
    int hr = ptrace(request, m_pid, addr, data);
    if (hr == -1) {
      std::cerr << "Oh dear, something went wrong with" << __PRETTY_FUNCTION__
                << ", error =  " << strerror(errno)
                << ", request = " << request
                << ", pid = " << m_pid
                << ", addr = " << addr << "\n";
      return hr;
    } else {
      std::cout << "Good with ptrace, hr = " << hr << "\n";
    }
    return 0;
  }

#endif