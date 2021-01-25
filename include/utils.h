#pragma once

#include <string>
#include <sys/ptrace.h>

bool is_prefix(const std::string& s, const std::string& of);
uint64_t m_ptrace(__ptrace_request request, pid_t m_pid, uint64_t addr, uint64_t* data);