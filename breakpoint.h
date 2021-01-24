#pragma once

#include <sys/types.h>
#include <cstdint>

template<class AddrT>
class BreakPoint {
public:
  BreakPoint() : m_pid(0), m_addr(0), m_enabled(false), m_saved_data(0) {};
  BreakPoint(pid_t pid, AddrT addr)
      : m_pid(pid), m_addr(addr), m_enabled(false), m_saved_data(0)
  {}

  void enable();
  void disable();

  auto is_enabled() const -> bool { return m_enabled; }
  auto get_address() const -> std::intptr_t { return m_addr; }

private:
  pid_t m_pid;
  AddrT m_addr;
  bool m_enabled;
  uint8_t m_saved_data; //data which used to be at the BreakPoint address
};