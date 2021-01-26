#pragma once

#include <unordered_map>

#include "breakpoint.h"

class Debugger {
  std::string m_prog_name;
  pid_t m_pid;

  std::unordered_map<uint64_t, BreakPoint> m_breakpoints;

public:
  Debugger(std::string prog_name, pid_t pid): m_prog_name(std::move(prog_name)), m_pid(pid) {}

  ~Debugger() {
    dispose();
  }

  void dispose();

  void wait_for_signal();
  void run();
  void handle_command(const char* command);
  void continue_execution();
  void step_over_breakpoint();

  void set_breakpoint_at_address(uint64_t addr);
  void dump_registers();

  uint64_t get_pc();
  void set_pc(uint64_t pc);

  static uint64_t convert_arg_to_hex_address(const std::string& arg);
};
