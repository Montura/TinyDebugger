#include <iostream>
#include <vector>
#include <iomanip>
#include <fstream>

#if __linux__
  #include <wait.h>
#endif

#include "linenoise.h"
#include "debugger.h"
#include "ptrace_impl.h"
#include "registers.h"

template <class Output>
void split(const std::string &s, char delimiter, Output result) {
  std::istringstream iss(s);
  std::string item;
  for (int i = 0; std::getline(iss, item, delimiter); ++i) {
    *result++ = item;
  }
}

bool is_prefix(const std::string& s, const std::string& of) {
  if (s.size() > of.size()) {
    return false;
  }
  return !s.empty() && std::equal(s.begin(), s.end(), of.begin());
}

Debugger::Debugger(std::string prog_name, pid_t pid) :
    m_prog_name(std::move(prog_name)),
    m_pid(pid),
    m_load_address(0)
{
  // open is used instead of std::ifstream because the elf loader needs a UNIX file descriptor to pass
  // to mmap so that it can map the file into memory rather than reading it a bit at a time.
  file_descriptor = open(m_prog_name.c_str(), O_RDONLY);

  // g++ -g helloworld.cpp -o helloworld (-g for generating DWARF)
  m_elf = elf::elf{elf::create_mmap_loader(file_descriptor)};
  m_dwarf = dwarf::dwarf{dwarf::elf::create_loader(m_elf)};
}

Debugger::~Debugger() {
  dispose();
  close(file_descriptor);
}

// Assume that the user has written 0xADDRESS (arg = 0xADDRESS)
uint64_t Debugger::convert_arg_to_hex_address(const std::string& arg) {
  std::string addr { arg, 2 }; // Cut 0x from the address;
  return std::stol(addr, nullptr, 16);
}

void Debugger::run() {
  std::cout << "Debugger::run -> Before waitpid() on pid = " << m_pid << "\n";
  wait_for_signal();
  initialize_load_address();
  std::cout << "Debugger::run -> After waitpid() on pid = " << m_pid << "\n";
  std::cout << "Debugee loaded at the address: " << (void*)m_load_address << "\n";

  char* line;
  while ((line = linenoise("minidbg> ")) != nullptr) {
    handle_command(line);
    linenoiseHistoryAdd(line);
    linenoiseFree(line);
  }
}

void Debugger::handle_command(const char* line) {
  std::vector<std::string> args;
  split(line, ' ', std::back_inserter(args));
//  std::cout << "Size of args:" << args.size() << "\n";
  auto command = args.empty() ? "" : args[0];

  if (is_prefix(command, "continue")) {
    continue_execution();
  } else if(is_prefix(command, "break")) {
    set_breakpoint_at_address(convert_arg_to_hex_address(args[1]));
  } else if (is_prefix(command, "register")) {
    if (is_prefix(args[1], "dump")) {
      dump_registers();
    } else if (is_prefix(args[1], "read")) {
      std::cout << getRegisterValue(m_pid, getRegisterByName(args[2])) << std::endl;
    } else if (is_prefix(args[1], "write")) {   // assume 0xVAL
      setRegisterValue(m_pid,
                       getRegisterByName(args[2]),
                       convert_arg_to_hex_address(args[3]));
    }
  } else if (is_prefix(command, "memory")) {
    if (is_prefix(args[1], "read")) {
      std::cout << std::hex
      << Ptrace::read_memory(m_pid, convert_arg_to_hex_address(args[2]))
      << std::endl;
    }
    if (is_prefix(args[1], "write")) {
      Ptrace::write_memory(m_pid,
                           convert_arg_to_hex_address(args[2]),
                           convert_arg_to_hex_address(args[3]));
    }
  } else if(is_prefix(command, "stepi")) {
    single_step_instruction_with_breakpoint_check();
    auto line_entry = get_line_entry_from_pc(get_pc());
    print_source(line_entry->file->path, line_entry->line);
  } else if(is_prefix(command, "step")) {
    step_in();
  } else if(is_prefix(command, "next")) {
    step_over();
  } else if(is_prefix(command, "finish")) {
    step_out();
  } else {
    std::cerr << "Unknown command\n";
  }
}

void Debugger::continue_execution() {
  step_over_breakpoint();
  // MacOS: error =  Operation not supported, request = 7, pid = 31429, addr = Segmentation fault: 11
  // Possible way to fix https://www.jetbrains.com/help/clion/attaching-to-local-process.html#prereq-ubuntu (solution for Ubuntu)
  Ptrace::continue_exec(m_pid);
  wait_for_signal();
}

void Debugger::set_breakpoint_at_address(uint64_t addr) {
  std::cout << "Set BreakPoint at address " << std::hex << addr << std::endl;
  BreakPoint bp {m_pid, addr};
  bp.enable();
  m_breakpoints[addr] = bp;
//  std::cout << "Added break point bp " << bp.get_address() << ", map size = " << m_breakpoints.size() << "\n";
}

void Debugger::dispose()  {
  for (auto & break_point : m_breakpoints) {
    break_point.second.disable();
  }
  m_breakpoints.clear();
}

void Debugger::dump_registers() {
  for (const auto& rd : globalRegisterDescriptors) {
    std::cout << rd.name << " 0x" << std::setfill('0') << std::setw(16)
              << std::hex << getRegisterValue(m_pid, rd.r)
              << std::endl;
  }
}

uint64_t Debugger::get_pc() {
  const uint64_t value = getRegisterValue(m_pid, Reg::rip);
  std::cout << "get_pc, pc = " << value << "\n";
  return value;
}

void Debugger::set_pc(uint64_t pc) {
  std::cout << "set_pc, pc = " << pc << "\n";
  setRegisterValue(m_pid, Reg::rip, pc);
}

void Debugger::step_over_breakpoint() {
  const uint64_t pc = get_pc();
  std::cout << "step_over_breakpoint, pc = " << pc << "\n";
  if (m_breakpoints.count(pc)) {
    auto& bp = m_breakpoints[pc];
    if (bp.is_enabled()) {
      bp.disable();
      Ptrace::single_step(m_pid);
      wait_for_signal();
      bp.enable();
    }
  }
}

void Debugger::wait_for_signal() {
  int wait_status;
  auto options = 0;
  waitpid(m_pid, &wait_status, options);

  auto siginfo = get_signal_info();

  switch (siginfo.si_signo) {
    case SIGTRAP:
      handle_sigtrap(siginfo);
      break;
    case SIGSEGV:
      std::cout << "Yay, segfault. Reason: " << siginfo.si_code << std::endl;
      break;
    default:
      std::cout << "Got signal " << strsignal(siginfo.si_signo) << std::endl;
  }
}

dwarf::die Debugger::get_function_from_pc(uint64_t pc) {
  auto offset_pc = offset_load_address(pc); // remember to offset the pc for querying DWARF

  std::cerr  << "get_function_from_pc, pc = " << offset_pc << "\n";
  // We find the correct compilation unit, then ask the line table to get us the relevant entry.
  for (const auto &compilationUnit : m_dwarf.compilation_units()) {
    if (die_pc_range(compilationUnit.root()).contains(offset_pc)) {
      for (const auto& die : compilationUnit.root()) {
        if (die.tag == dwarf::DW_TAG::subprogram) {
          if (die_pc_range(die).contains(offset_pc)) {
            return die;
          }
        }
      }
    }
  }
  throw std::out_of_range{"Cannot find function"};
}

dwarf::line_table::iterator Debugger::get_line_entry_from_pc(const uint64_t& pc, bool need_offset) {
  auto offset_pc = need_offset ? offset_load_address(pc) : pc; // remember to offset the pc for querying DWARF

  std::cerr  << "get_line_entry_from_pc, pc = " << offset_pc << "\n";
  for (auto &compilationUnit : m_dwarf.compilation_units()) {
    if (die_pc_range(compilationUnit.root()).contains(offset_pc)) {
      const auto &tableLine = compilationUnit.get_line_table();
      auto it = tableLine.find_address(offset_pc);
      if (it == tableLine.end()) {
        throw std::out_of_range { "Cannot find line entry" };
      } else {
        return it;
      }
    }
  }

  throw std::out_of_range { "Cannot find line entry" };
}

void Debugger::initialize_load_address() {
  // If this is a dynamic library (e.g. PIE)
  if (m_elf.get_hdr().type == elf::et::dyn) {
    // The load address is found in /proc/pid/maps
    std::ifstream map("/proc/" + std::to_string(m_pid) + "/maps");

    // Read the first address from the file
    std::string addr;
    std::getline(map, addr, '-');

    m_load_address = std::stol(addr, nullptr, 16);
  }
}

uint64_t Debugger::offset_load_address(uint64_t addr) {
  return addr - m_load_address;
}

uint64_t Debugger::offset_dwarf_address(uint64_t dwarf_addr) {
  return dwarf_addr + m_load_address;
}

// todo: check the value of n_lines_context
void Debugger::print_source(const std::string& file_name, uint32_t line, uint32_t n_lines_context) {
  std::ifstream file { file_name };

  // Work out a window around the desired line
  uint32_t start_line = (line <= n_lines_context) ? 1 : line - n_lines_context;
  uint32_t end_line = line + n_lines_context + (line < n_lines_context ? n_lines_context - line : 0) + 1;

  char c = 0;
  uint32_t current_line = 1;
  // Skip lines up until start_line
  while (current_line != start_line && file.get(c)) {
    if (c == '\n') {
      ++current_line;
    }
  }

  // Output cursor if we're at the current line
  std::cout << (current_line==line ? "> " : "  ");

  // Write lines up until end_line
  while (current_line <= end_line && file.get(c)) {
    std::cout << c;
    if (c == '\n') {
      ++current_line;
      // Output cursor if we're at the current line
      std::cout << (current_line==line ? "> " : "  ");
    }
  }

  // Write newline and make sure that the stream is flushed properly
  std::cout << std::endl;
}

siginfo_t Debugger::get_signal_info() {
  siginfo_t info;
  Ptrace::get_sig_info(m_pid, &info);
  return info;
}

void Debugger::handle_sigtrap(siginfo_t const& info) {
  switch (info.si_code) {
    // one of these will be set if a breakpoint was hit
    case SI_KERNEL:
    case TRAP_BRKPT:
    {
      set_pc(get_pc() - 1); // put the pc back where it should be
      std::cout << "Hit breakpoint at address " << std::hex << get_pc() << std::endl;
      auto line_entry = get_line_entry_from_pc(get_pc());
      print_source(line_entry->file->path, line_entry->line);
      return;
    }
      // this will be set if the signal was sent by single stepping
    case TRAP_TRACE:
      return;
    default:
      std::cout << "Unknown SIGTRAP code " << info.si_code << std::endl;
      return;
  }
}

void Debugger::single_step_instruction() {
  Ptrace::single_step(m_pid);
  wait_for_signal();
}

void Debugger::single_step_instruction_with_breakpoint_check() {
  // first, check to see if we need to disable and enable a breakpoint
  if (m_breakpoints.count(get_pc())) {
    std::cout << "step_over_breakpoint\n";
    step_over_breakpoint();
  } else {
    std::cout << "single_step_instruction\n";
    single_step_instruction();
  }
}

void Debugger::step_out() {
  //  Return address is stored 8 bytes after the start of a stack frame.
  uint64_t return_address = getReturnAddress();

  bool should_remove_breakpoint = false;
  if (!m_breakpoints.count(return_address)) {
    set_breakpoint_at_address(return_address);
    should_remove_breakpoint = true;
  }

  continue_execution();

  if (should_remove_breakpoint) {
    remove_breakpoint(return_address);
  }
}

void Debugger::remove_breakpoint(uint64_t addr) {
  BreakPoint& point = m_breakpoints.at(addr);
  if (point.is_enabled()) {
    point.disable();
  }
  m_breakpoints.erase(addr);
}

void Debugger::step_in() {
  const uint64_t pc = get_pc();
  auto dwarf_table_line = get_line_entry_from_pc(pc)->line;

  while (get_line_entry_from_pc(pc)->line == dwarf_table_line) {
    single_step_instruction_with_breakpoint_check();
  }

  auto line_entry = get_line_entry_from_pc(pc);
  print_source(line_entry->file->path, line_entry->line);
}

// Real debuggers will often examine what instruction is being executed and
// work out all of the possible branch targets, then set breakpoints on all of them.
// We’ll need to come up with a simpler solution.
//  1) Stepping until we’re at a new line in the current function
//  2) To set a breakpoint at every line in the current function.
// Solution:
//  Consider the 1st one.
//  If we’re stepping over a function call, we need to single step through every single instruction
//  in that call graph.
//  So use the 2nd approach
void Debugger::step_over() {
  const uint64_t pc = get_pc();
  dwarf::die const func_die = get_function_from_pc(pc);
  uint64_t func_entry = at_low_pc(func_die); // DW_AT::low_pc
  uint64_t func_end = at_high_pc(func_die); // DW_AT::high_pc

  auto curr_line = get_line_entry_from_pc(func_entry, false);
  auto start_line = get_line_entry_from_pc(pc);

  std::vector<uint64_t> to_delete;

  while(curr_line->address < func_end) {
    const uint64_t current_address = curr_line->address;
    const uint64_t load_address = offset_dwarf_address(current_address);
    if (current_address != start_line->address && !m_breakpoints.count(load_address)) {
      set_breakpoint_at_address(load_address);
      to_delete.push_back(load_address);
    }
    ++curr_line;
  }

  // Set a breakpoint on the return address of the function, just like in step_out.
  uint64_t return_address = getReturnAddress();
  if (!m_breakpoints.count(return_address)) {
    set_breakpoint_at_address(return_address);
    to_delete.push_back(return_address);
  }

  continue_execution();

  for (auto addr : to_delete) {
    remove_breakpoint(addr);
  }
}

uint64_t Debugger::getReturnAddress() const {
  uint64_t frame_pointer = getRegisterValue(m_pid, Reg::rbp);
  return Ptrace::read_memory(m_pid, frame_pointer + 8);
}
