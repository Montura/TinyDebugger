#pragma once

#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <csignal>

#include "breakpoint.h"
#include "internal.hh"
#include "elf++.hh"
#include "symbol.h"

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

  void waitForSignal();
  void run();
  void handleCommand(const char* command);
  void continueExecution();
  void stepOverBreakpoint();

  void setBreakpointAtAddress(uint64_t addr);
  void dumpRegisters();

  uint64_t getPc();
  void setPc(uint64_t pc);

  static uint64_t convertArgToHexAddress(const std::string& arg);

  // DWARF ->  standardized debugging data format
  // ELF -> Executable and Linkage Format:
  //    1. http://www.skyfree.org/linux/references/ELF_Format.pdf
  //    2. https://raw.githubusercontent.com/corkami/pics/master/binary/elf101/elf101-64.pdf
  // DIE -> DWARF Information Entry:
  //    1. http://www.dwarfstd.org/doc/Debugging%20using%20DWARF-2012.pdf
  //    2. https://blog.tartanllama.xyz/writing-a-linux-debugger-elf-dwarf/
  dwarf::die getFunctionFromPc(uint64_t pc);
  dwarf::line_table::iterator getLineEntryFromPc(const uint64_t& pc, bool need_offset = true);

  void initializeLoadAddress();
  uint64_t offsetLoadAddress(uint64_t addr);
  void printSource(const std::string& file_name, uint32_t line, uint32_t n_lines_context = 2);
  siginfo_t getSignalInfo();
  void handleSigtrap(siginfo_t const& info);

  void singleStepInstruction();
  void singleStepInstructionWithBreakpointCheck();
  void stepOver();
  void stepOut();
  void stepIn();

  void removeBreakpoint(uint64_t addr);
  uint64_t offsetDwarfAddress(uint64_t dwarf_addr);
  uint64_t getReturnAddress() const;

  void setBreakpointAtFunction(const std::string& name);

  void setBreakpointAtSourceLine(const std::string& file_name, uint32_t line_number);

  std::vector<Symbol> lookupSymbol(const std::string& name);
};
