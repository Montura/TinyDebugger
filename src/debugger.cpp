#include <iostream>
#include <vector>
#include <iomanip>

#if __linux__
  #include <wait.h>
#endif

#include "linenoise.h"
#include "debugger.h"
#include "utils.h"
#include "registers.h"

template <class Output>
void split(const std::string &s, char delimiter, Output result) {
  std::istringstream iss(s);
  std::string item;
  for (int i = 0; std::getline(iss, item, delimiter); ++i) {
    *result++ = item;
  }
}

void Debugger::run() {
  std::cout << "Debugger::run -> Before waitpid() on pid = " << m_pid << "\n";
  wait_for_signal();
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
    std::string addr {args[1], 2}; // naively assume that the user has written 0xADDRESS
    set_breakpoint_at_address(std::stol(addr, 0, 16));
  } else if (is_prefix(command, "register")) {
    if (is_prefix(args[1], "dump")) {
      dump_registers();
    } else if (is_prefix(args[1], "read")) {
      std::cout << getRegisterValue(m_pid, getRegisterFromName(args[2])) << std::endl;
    } else if (is_prefix(args[1], "write")) {
      std::string val{args[3], 2}; //assume 0xVAL
      setRegisterValue(m_pid, getRegisterFromName(args[2]), std::stol(val, 0, 16));
    }
  } else if (is_prefix(command, "memory")) {
    std::string addr{args[2], 2}; //assume 0xADDRESS

    if (is_prefix(args[1], "read")) {
      std::cout << std::hex << read_memory(std::stol(addr, 0, 16)) << std::endl;
    }
    if (is_prefix(args[1], "write")) {
      std::string val{args[3], 2}; //assume 0xVAL
      write_memory(std::stol(addr, 0, 16), std::stol(val, 0, 16));
    }
  } else {
    std::cerr << "Unknown command\n";
  }
}

void Debugger::continue_execution() {
  step_over_breakpoint();
  // MacOS: error =  Operation not supported, request = 7, pid = 31429, addr = Segmentation fault: 11
  // Possible way to fix https://www.jetbrains.com/help/clion/attaching-to-local-process.html#prereq-ubuntu (solution for Ubuntu)
  m_ptrace(PT_CONTINUE, m_pid, 0, 0);
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

uint64_t Debugger::read_memory(uint64_t address) {
  return m_ptrace(PTRACE_PEEKDATA, m_pid, address, nullptr);
}

void Debugger::write_memory(uint64_t address, uint64_t value) {
  m_ptrace(PTRACE_POKEDATA, m_pid, address, reinterpret_cast<uint64_t*>(value));
}

uint64_t Debugger::get_pc() {
  return getRegisterValue(m_pid, Reg::rip);
}

void Debugger::set_pc(uint64_t pc) {
  setRegisterValue(m_pid, Reg::rip, pc);
}

void Debugger::step_over_breakpoint() {
  // - 1 because execution will go past the breakpoint
  auto possible_breakpoint_location = get_pc() - 1;

  if (m_breakpoints.count(possible_breakpoint_location)) {
    auto& bp = m_breakpoints[possible_breakpoint_location];

    if (bp.is_enabled()) {
      auto previous_instruction_address = possible_breakpoint_location;
      set_pc(previous_instruction_address);

      bp.disable();
      ptrace(PTRACE_SINGLESTEP, m_pid, nullptr, nullptr);
      wait_for_signal();
      bp.enable();
    }
  }
}

void Debugger::wait_for_signal() {
  int wait_status;
  auto options = 0;
  waitpid(m_pid, &wait_status, options);
}