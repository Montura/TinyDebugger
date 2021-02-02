#pragma once

#include <sys/types.h>
#include <cstdint>

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