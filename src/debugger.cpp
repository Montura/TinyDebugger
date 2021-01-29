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
      std::cout << getRegisterValue(m_pid, getRegisterFromName(args[2])) << std::endl;
    } else if (is_prefix(args[1], "write")) {   // assume 0xVAL
      setRegisterValue(m_pid,
                       getRegisterFromName(args[2]),
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
  return getRegisterValue(m_pid, Reg::rip);
}

void Debugger::set_pc(uint64_t pc) {
  setRegisterValue(m_pid, Reg::rip, pc);
}

void Debugger::step_over_breakpoint() {
  const uint64_t pc = get_pc();
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
  // We find the correct compilation unit, then ask the line table to get us the relevant entry.
  for (const auto &compilationUnit : m_dwarf.compilation_units()) {
    if (die_pc_range(compilationUnit.root()).contains(pc)) {
      for (const auto& die : compilationUnit.root()) {
        if (die.tag == dwarf::DW_TAG::subprogram) {
          if (die_pc_range(die).contains(pc)) {
            return die;
          }
        }
      }
    }
  }
  throw std::out_of_range{"Cannot find function"};
}

dwarf::line_table::iterator Debugger::get_line_entry_from_pc(uint64_t pc) {
  for (auto &compilationUnit : m_dwarf.compilation_units()) {
    if (die_pc_range(compilationUnit.root()).contains(pc)) {
      const auto &tableLine = compilationUnit.get_line_table();
      auto it = tableLine.find_address(pc);
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
// todo: check the value of n_lines_context
void Debugger::print_source(const std::string& file_name, uint32_t line, uint32_t n_lines_context = 2) {
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
      std::cout << "Hit breakpoint at address 0x" << std::hex << get_pc() << std::endl;
      auto offset_pc = offset_load_address(get_pc()); // remember to offset the pc for querying DWARF
      auto line_entry = get_line_entry_from_pc(offset_pc);
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