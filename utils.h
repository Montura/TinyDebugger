#pragma once

#include <string>
#include <sys/ptrace.h>

bool is_prefix(const std::string& s, const std::string& of);

#if __linux__
long m_ptrace(__ptrace_request request, pid_t m_pid, void* addr, uint64_t data);
#elif __APPLE__
int m_ptrace(int request, pid_t m_pid, caddr_t addr, uint64_t data);
#endif