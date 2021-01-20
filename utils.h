#pragma once

#include <string>
#include <sys/ptrace.h>

bool is_prefix(const std::string& s, const std::string& of);

int m_ptrace(int request, pid_t m_pid, caddr_t addr, int data);