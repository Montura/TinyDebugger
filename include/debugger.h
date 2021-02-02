#pragma once

#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <csignal>

#include "breakpoint.h"
#include "internal.hh"
#include "elf++.hh"

class Debugger {
  std::string m_prog_name;
  pid_t m_pid;

  std::unordered_map<uint64_t, BreakPoint> m_breakpoints;

  dwarf::dwarf m_dwarf;
  elf::elf m_elf;
  int file_descriptor;
  uint64_t m_load_address;

public:
  Debugger(std::string prog_name, pid_t pid);
  ~Debugger();

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

  // DWARF ->  standardized debugging data format
  // ELF -> Executable and Linkage Format:
  //    1. http://www.skyfree.org/linux/references/ELF_Format.pdf
  //    2. https://raw.githubusercontent.com/corkami/pics/master/binary/elf101/elf101-64.pdf
  // DIE -> DWARF Information Entry:
  //    1. http://www.dwarfstd.org/doc/Debugging%20using%20DWARF-2012.pdf
  //    2. https://blog.tartanllama.xyz/writing-a-linux-debugger-elf-dwarf/
  dwarf::die get_function_from_pc(uint64_t pc);
  dwarf::line_table::iterator get_line_entry_from_pc(const uint64_t& pc, bool need_offset = true);

  void initialize_load_address();
  uint64_t offset_load_address(uint64_t addr);
  void print_source(const std::string& file_name, uint32_t line, uint32_t n_lines_context = 2);
  siginfo_t get_signal_info();
  void handle_sigtrap(siginfo_t const& info);

  void single_step_instruction();
  void single_step_instruction_with_breakpoint_check();
  void step_over();
  void step_out();
  void step_in();

  void remove_breakpoint(uint64_t addr);
  uint64_t offset_dwarf_address(uint64_t dwarf_addr);
  uint64_t getReturnAddress() const;
};
