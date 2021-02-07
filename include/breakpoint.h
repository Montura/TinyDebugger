#pragma once

#include <sys/types.h>
#include <cstdint>

// So the questions are:
//
// 1. How do we modify the code?
//    - ptrace (use it to read and write memory)

// 2. What modifications do we make to set a breakpoint?
//    1) We have to cause the processor to halt and signal the program when the breakpoint ADDRESS is executed.
//    On x86 the first byte of ADDRESS is overwriting with the "int 3" instruction.
//    2) x86 has an interrupt vector table. When the processor executes the "int 3" instruction, control is passed to
//    the breakpoint interrupt handler, which (in the case of Linux) signals the process with a "SIGTRAP".

// 3. How is the debugger notified?
//    We used waitpid to listen for signals which are sent to the debugee.
//    Do exactly the same thing here: set the breakpoint, continue the program, call waitpid and wait until the SIGTRAP occurs.

class BreakPoint {
public:
  BreakPoint() : m_pid(0), m_addr(0), m_enabled(false), m_saved_data(0) {};
  BreakPoint(pid_t pid, uint64_t addr)
      : m_pid(pid), m_addr(addr), m_enabled(false), m_saved_data(0)
  {}

  void enable();
  void disable();

  auto isEnabled() const -> bool { return m_enabled; }
  auto getAddress() const -> uint64_t { return m_addr; }

private:
  pid_t m_pid;
  uint64_t m_addr;
  bool m_enabled;
  uint8_t m_saved_data; // data which used to be at the BreakPoint address
};