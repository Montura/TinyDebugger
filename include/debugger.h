#pragma once

#include <unordered_map>

#include "breakpoint.h"

class Debugger {
  std::string m_prog_name;
  pid_t m_pid;

  std::unordered_map<uint64_t, BreakPoint> m_breakpoints;

public:
  Debugger(std::string prog_name, pid_t pid): m_prog_name(std::move(prog_name)), m_pid(pid) {}

  void run();
  void handle_command(const char* command);
  void continue_execution();
  void set_breakpoint_at_address(uint64_t addr);
  void dispose();
  void dump_registers();

  ~Debugger() {
    dispose();
  }
};
