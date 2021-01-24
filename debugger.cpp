#include <iostream>
#include <sstream>
#include <vector>
#if __linux__
#include <wait.h>
#endif

#include "linenoise/linenoise.h"
#include "debugger.h"
#include "utils.h"

template <class Output>
void split(const std::string &s, char delimiter, Output result) {
  std::istringstream iss(s);
  std::string item;
  for (int i = 0; std::getline(iss, item, delimiter); ++i) {
    *result++ = item;
  }
}

template <class AddrT>
void Debugger<AddrT>::run() {
  int wait_status;
  auto options = 0;
  std::cout << "Debugger::run -> Before waitpid() on pid = " << m_pid << "\n";
  waitpid(m_pid, &wait_status, options);
  std::cout << "Debugger::run -> After waitpid() on pid = " << m_pid << "\n";

  char* line;
  while ((line = linenoise("minidbg> ")) != nullptr) {
    handle_command(line);
    linenoiseHistoryAdd(line);
    linenoiseFree(line);
  }
}

template <class AddrT>
void Debugger<AddrT>::handle_command(const char* line) {
  std::vector<std::string> args;
  split(line, ' ', std::back_inserter(args));
//  std::cout << "Size of args:" << args.size() << "\n";
  auto command = args.empty() ? "" : args[0];

  if (is_prefix(command, "continue")) {
    continue_execution();
  } else if(is_prefix(command, "break")) {
    std::string addr {args[1], 2}; // naively assume that the user has written 0xADDRESS
    set_breakpoint_at_address(reinterpret_cast<AddrT>(std::stol(addr, 0, 16)));
  } else {
    std::cerr << "Unknown command\n";
  }
}

template <class AddrT>
void Debugger<AddrT>::continue_execution() {
  // MacOS: error =  Operation not supported, request = 7, pid = 31429, addr = Segmentation fault: 11
  // Possible way to fix https://www.jetbrains.com/help/clion/attaching-to-local-process.html#prereq-ubuntu (solution for Ubuntu)
  m_ptrace(PT_CONTINUE, m_pid, nullptr, 0);

  int wait_status;
  auto options = 0;
  waitpid(m_pid, &wait_status, options);
}

template <class AddrT>
void Debugger<AddrT>::set_breakpoint_at_address(AddrT addr) {
  std::cout << "Set BreakPoint at address " << std::hex << addr << std::endl;
  BreakPoint<AddrT> bp {m_pid, addr};
  bp.enable();
  m_breakpoints[addr] = bp;
//  std::cout << "Added break point bp " << bp.get_address() << ", map size = " << m_breakpoints.size() << "\n";
}