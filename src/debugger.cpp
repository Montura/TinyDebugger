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
#include "symbol.h"

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

bool is_suffix(const std::string& s, const std::string& of) {
  if (s.size() > of.size()) {
    return false;
  }
  size_t diff = of.size() - s.size();
  return std::equal(s.begin(), s.end(), of.begin() + diff);
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
uint64_t Debugger::convertArgToHexAddress(const std::string& arg) {
  std::string addr { arg, 2 }; // Cut 0x from the address;
  return std::stol(addr, nullptr, 16);
}

void Debugger::run() {
  std::cout << "Debugger::run -> Before waitpid() on pid = " << m_pid << "\n";
  waitForSignal();
  initializeLoadAddress();
  std::cout << "Debugger::run -> After waitpid() on pid = " << m_pid << "\n";
  std::cout << "Debugee loaded at the address: " << (void*)m_load_address << "\n";

  char* line;
  while ((line = linenoise("minidbg> ")) != nullptr) {
    handleCommand(line);
    linenoiseHistoryAdd(line);
    linenoiseFree(line);
  }
}

void Debugger::handleCommand(const char* line) {
  std::vector<std::string> args;
  split(line, ' ', std::back_inserter(args));
//  std::cout << "Size of args:" << args.size() << "\n";
  auto command = args.empty() ? "" : args[0];

  if (is_prefix(command, "continue")) {
    continueExecution();
  } else if(is_prefix(command, "break")) {
    if (args[1][0] == '0' && args[1][1] == 'x') {
      setBreakpointAtAddress(convertArgToHexAddress(args[1]));
    } else if (args[1].find(':') != std::string::npos) {
      std::vector<std::string> file_and_line;
      split(args[1], ':', std::back_inserter(file_and_line));
      setBreakpointAtSourceLine(file_and_line[0], std::stoi(file_and_line[1]));
    } else {
      setBreakpointAtFunction(args[1]);
    }
  } else if (is_prefix(command, "register")) {
    if (is_prefix(args[1], "dump")) {
      dumpRegisters();
    } else if (is_prefix(args[1], "read")) {
      std::cout << getRegisterValue(m_pid, getRegisterByName(args[2])) << std::endl;
    } else if (is_prefix(args[1], "write")) {   // assume 0xVAL
      setRegisterValue(m_pid,
                       getRegisterByName(args[2]),
                       convertArgToHexAddress(args[3]));
    }
  } else if (is_prefix(command, "memory")) {
    if (is_prefix(args[1], "read")) {
      std::cout << std::hex
      << Ptrace::readMemory(m_pid, convertArgToHexAddress(args[2]))
      << std::endl;
    }
    if (is_prefix(args[1], "write")) {
      Ptrace::writeMemory(m_pid,
                          convertArgToHexAddress(args[2]),
                          convertArgToHexAddress(args[3]));
    }
  } else if(is_prefix(command, "stepi")) {
    singleStepInstructionWithBreakpointCheck();
    auto line_entry = getLineEntryFromPc(getPc());
    printSource(line_entry->file->path, line_entry->line);
  } else if(is_prefix(command, "step")) {
    stepIn();
  } else if(is_prefix(command, "next")) {
    stepOver();
  } else if(is_prefix(command, "finish")) {
    stepOut();
  } else if(is_prefix(command, "symbol")) {
    auto symbol_vector = lookupSymbol(args[1]);
    for (const Symbol& symbol : symbol_vector) {
      std::cout << symbol;
    }
  } else if(is_prefix(command, "backtrace")) {
    printBacktrace();
  } else {
    std::cerr << "Unknown command\n";
  }
}

void Debugger::continueExecution() {
  stepOverBreakpoint();
  // MacOS: error =  Operation not supported, request = 7, pid = 31429, addr = Segmentation fault: 11
  // Possible way to fix https://www.jetbrains.com/help/clion/attaching-to-local-process.html#prereq-ubuntu (solution for Ubuntu)
  Ptrace::continueExec(m_pid);
  waitForSignal();
}

// Debugger Part 2: Breakpoints
// https://blog.tartanllama.xyz/writing-a-linux-debugger-breakpoints/

void Debugger::setBreakpointAtAddress(uint64_t addr) {
  std::cout << "Set BreakPoint at address " << std::hex << addr << std::endl;
  BreakPoint bp {m_pid, addr};
  bp.enable();
  m_breakpoints[addr] = bp;
//  std::cout << "Added break point bp " << bp.getAddress() << ", map size = " << m_breakpoints.size() << "\n";
}

void Debugger::dispose()  {
  for (auto & break_point : m_breakpoints) {
    break_point.second.disable();
  }
  m_breakpoints.clear();
}

// Debugger Part 3: Registers and memory
// https://blog.tartanllama.xyz/writing-a-linux-debugger-registers/

void Debugger::dumpRegisters() {
  for (const auto& rd : globalRegisterDescriptors) {
    std::cout << rd.name << " 0x" << std::setfill('0') << std::setw(16)
              << std::hex << getRegisterValue(m_pid, rd.r)
              << std::endl;
  }
}

uint64_t Debugger::getPc() {
  const uint64_t value = getRegisterValue(m_pid, Reg::rip);
  std::cout << "getPc, pc = " << value << "\n";
  return value;
}

void Debugger::setPc(uint64_t pc) {
  std::cout << "setPc, pc = " << pc << "\n";
  setRegisterValue(m_pid, Reg::rip, pc);
}

void Debugger::stepOverBreakpoint() {
  const uint64_t pc = getPc();
  std::cout << "stepOverBreakpoint, pc = " << pc << "\n";
  if (m_breakpoints.count(pc)) {
    auto& bp = m_breakpoints[pc];
    if (bp.isEnabled()) {
      bp.disable();
      Ptrace::singleStep(m_pid);
      waitForSignal();
      bp.enable();
    }
  }
}

void Debugger::waitForSignal() {
  int wait_status;
  auto options = 0;
  waitpid(m_pid, &wait_status, options);

  auto siginfo = getSignalInfo();

  switch (siginfo.si_signo) {
    case SIGTRAP:
      handleSigtrap(siginfo);
      break;
    case SIGSEGV:
      std::cout << "Yay, segfault. Reason: " << siginfo.si_code << std::endl;
      break;
    default:
      std::cout << "Got signal " << strsignal(siginfo.si_signo) << std::endl;
  }
}

// Debugger Part 5: Source and signals
// https://blog.tartanllama.xyz/writing-a-linux-debugger-source-signal/

dwarf::die Debugger::getFunctionFromPc(uint64_t pc) {
  auto offset_pc = offsetLoadAddress(pc); // remember to offset the pc for querying DWARF

  std::cerr  << "getFunctionFromPc, pc = " << offset_pc << "\n";
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

dwarf::line_table::iterator Debugger::getLineEntryFromPc(const uint64_t& pc, bool need_offset) {
  auto offset_pc = need_offset ? offsetLoadAddress(pc) : pc; // remember to offset the pc for querying DWARF

//  std::cerr  << "getLineEntryFromPc, pc = " << offset_pc << "\n";
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

void Debugger::initializeLoadAddress() {
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

uint64_t Debugger::offsetLoadAddress(uint64_t addr) {
  return addr - m_load_address;
}

uint64_t Debugger::offsetDwarfAddress(uint64_t dwarf_addr) {
  return dwarf_addr + m_load_address;
}

// todo: check the value of n_lines_context
void Debugger::printSource(const std::string& file_name, uint32_t line, uint32_t n_lines_context) {
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

siginfo_t Debugger::getSignalInfo() {
  siginfo_t info;
  Ptrace::getSigInfo(m_pid, &info);
  return info;
}

void Debugger::handleSigtrap(siginfo_t const& info) {
  switch (info.si_code) {
    // one of these will be set if a breakpoint was hit
    case SI_KERNEL:
    case TRAP_BRKPT:
    {
      setPc(getPc() - 1); // put the pc back where it should be
      std::cout << "Hit breakpoint at address " << std::hex << getPc() << std::endl;
      auto line_entry = getLineEntryFromPc(getPc());
      printSource(line_entry->file->path, line_entry->line);
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

// Debugger Part 6: Source-level stepping
// https://blog.tartanllama.xyz/writing-a-linux-debugger-dwarf-step/

void Debugger::singleStepInstruction() {
  Ptrace::singleStep(m_pid);
  waitForSignal();
}

void Debugger::singleStepInstructionWithBreakpointCheck() {
  // first, check to see if we need to disable and enable a breakpoint
  if (m_breakpoints.count(getPc())) {
    std::cout << "stepOverBreakpoint\n";
    stepOverBreakpoint();
  } else {
    std::cout << "singleStepInstruction\n";
    singleStepInstruction();
  }
}

void Debugger::stepOut() {
  uint64_t return_address = getReturnAddress();

  bool should_remove_breakpoint = false;
  if (!m_breakpoints.count(return_address)) {
    setBreakpointAtAddress(return_address);
    should_remove_breakpoint = true;
  }

  continueExecution();

  if (should_remove_breakpoint) {
    removeBreakpoint(return_address);
  }
}

void Debugger::removeBreakpoint(uint64_t addr) {
  BreakPoint& point = m_breakpoints.at(addr);
  if (point.isEnabled()) {
    point.disable();
  }
  m_breakpoints.erase(addr);
}

void Debugger::stepIn() {
  const uint64_t pc = getPc();
  auto dwarf_table_line = getLineEntryFromPc(pc)->line;

  while (getLineEntryFromPc(pc)->line == dwarf_table_line) {
    singleStepInstructionWithBreakpointCheck();
  }

  auto line_entry = getLineEntryFromPc(pc);
  printSource(line_entry->file->path, line_entry->line);
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
void Debugger::stepOver() {
  const uint64_t pc = getPc();
  dwarf::die const func_die = getFunctionFromPc(pc);
  uint64_t func_entry = at_low_pc(func_die); // DW_AT::low_pc
  uint64_t func_end = at_high_pc(func_die); // DW_AT::high_pc

  auto curr_line = getLineEntryFromPc(func_entry, false);
  auto start_line = getLineEntryFromPc(pc);

  std::vector<uint64_t> to_delete;

  while(curr_line->address < func_end) {
    const uint64_t current_address = curr_line->address;
    const uint64_t load_address = offsetDwarfAddress(current_address);
    if (current_address != start_line->address && !m_breakpoints.count(load_address)) {
      setBreakpointAtAddress(load_address);
      to_delete.push_back(load_address);
    }
    ++curr_line;
  }

  uint64_t return_address = getReturnAddress();
  if (!m_breakpoints.count(return_address)) {
    setBreakpointAtAddress(return_address);
    to_delete.push_back(return_address);
  }

  continueExecution();

  for (auto addr : to_delete) {
    removeBreakpoint(addr);
  }
}

uint64_t Debugger::getReturnAddress() const {
  // Return address is stored 8 bytes after the start of a stack frame.
  uint64_t frame_pointer = getRegisterValue(m_pid, Reg::rbp);
  return Ptrace::readMemory(m_pid, frame_pointer + 8);
}

uint64_t Debugger::unwindFramePointer(uint64_t& frame_pointer) const {
  // Return address is stored 8 bytes after the start of a stack frame.
  frame_pointer = Ptrace::readMemory(m_pid, frame_pointer);
  return Ptrace::readMemory(m_pid, frame_pointer + 8);
}

// Debugger Part 7: Source-level breakpoints
// https://blog.tartanllama.xyz/writing-a-linux-debugger-source-break/
//    DWARF contains the address ranges of functions and a line table which lets you
// translate code positions between abstraction levels.

//  Function entry (onyly for global scope)
//  Idea:
//    Iterate through all of the CU and search for functions with names which match what we’re looking for.
void Debugger::setBreakpointAtFunction(const std::string& name) {
  for (const auto& compilation_unit : m_dwarf.compilation_units()) {
    for (const auto& die : compilation_unit.root()) {
      if (die.has(dwarf::DW_AT::name) && at_name(die) == name) {
        auto low_pc = at_low_pc(die);
        // DW_AT_low_pc for a function points to the start of the prologue.
        auto entry = getLineEntryFromPc(low_pc, false);
        // Hack: increment the line entry by one to get the first line of the user code instead of the prologue.
        ++entry;
        setBreakpointAtAddress(offsetDwarfAddress(entry->address));
      }
    }
  }
}

// Source line
//  Idea: Translate this line number into an address by looking it up in the DWARF.
//  1. Iterate through the CU looking for one whose name matches the given file.
//  2. Look for the entry which corresponds to the given line.
void Debugger::setBreakpointAtSourceLine(const std::string& file_name, uint32_t line_number) {
  for (const auto& compilation_unit : m_dwarf.compilation_units()) {
    if (is_suffix(file_name, at_name(compilation_unit.root()))) {
      const auto& line_table = compilation_unit.get_line_table();

      for (const auto& entry : line_table) {
        // entry.is_stmt is checking that the line table entry is marked as the beginning of a statement,
        // which is set by the compiler on the address it thinks is the best target for a breakpoint.
        if (entry.is_stmt && entry.line == line_number) {
          setBreakpointAtAddress(offsetDwarfAddress(entry.address));
          return;
        }
      }
    }
  }
}

// Symbol lookup
// Idea: take a look at the .symtab section of a binary, produced with readelf
std::vector<Symbol> Debugger::lookupSymbol(const std::string& name) {
  std::vector<Symbol> symbols;
  for (const auto &section : m_elf.sections()) {
    const elf::sht section_type = section.get_hdr().type;
    if (section_type != elf::sht::symtab && section_type != elf::sht::dynsym) {
      continue;
    }

    for (const elf::sym symbol : section.as_symtab()) {
      const std::string& symbol_name = symbol.get_name();
      if (symbol_name == name) {
        symbols.emplace_back(symbol.get_data(), symbol_name);
      }
    }
  }

  return symbols;
}

// Debugger Part 8: Stack unwinding

//  1. The most robust way to do this is to parse the .eh_frame section of the ELF file and work out
//  how to unwind the stack from there.
//  2. To use libunwind or something similar (step 1) to do it for you.
//  3. Walk the stack manually
//  Idea:
//    Assume that the compiler has laid out the stack in a certain.
//    In order to do this, we first need to understand how the stack is laid out:

//             High
//        |   ...   |
//        +---------+
//     +24|  Arg 1  |
//        +---------+
//     +16|  Arg 2  |
//        +---------+
//     + 8| Return  |
//        +---------+
//EBP+--> |Saved EBP|
//        +---------+
//     - 8|  Var 1  |
//        +---------+
//ESP+--> |  Var 2  |
//        +---------+
//        |   ...   |
//            Low

// lambda replacement
//void Debugger::logBacktraceLine(const dwarf::die& func_dwarf_addr) {
//  static uint32_t frame_number = 0;
//  std::cout << "frame #" << frame_number++ << ": 0x" << dwarf::at_low_pc(func_dwarf_addr)
//            << ' ' << dwarf::at_name(func_dwarf_addr) << std::endl;
//}

void Debugger::printBacktrace() {
  auto output_frame = [frame_number = 0] (auto&& func) mutable {
    std::cout << "frame #" << frame_number++ << ": 0x" << dwarf::at_low_pc(func)
              << ' ' << dwarf::at_name(func) << std::endl;
  };
  dwarf::die current_func = getFunctionFromPc(getPc());
  output_frame(current_func);

  uint64_t frame_pointer = getRegisterValue(m_pid, Reg::rbp);
  uint64_t return_address = Ptrace::readMemory(m_pid, frame_pointer + 8);

  // Options:
  // 1. To keep unwinding until the debugger hits main
  // 2. To stop when the frame pointer is 0x0, which will get you the functions which your implementation
  // called before main as well.
  // Idea:
  //  To grab the frame pointer and return address from each frame and print out the information as we go.
  while (dwarf::at_name(current_func) != "main") {
    current_func = getFunctionFromPc(return_address);
    output_frame(current_func);
    return_address = unwindFramePointer(frame_pointer);
  }
}