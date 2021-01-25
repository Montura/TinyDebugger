#include "breakpoint.h"
#include "utils.h"

#if __APPLE__
#define PTRACE_PEEKDATA PT_READ_D
#define PTRACE_POKEDATA PT_WRITE_D
#endif

void BreakPoint::enable() {
  uint64_t data = m_ptrace(PTRACE_PEEKDATA, m_pid, m_addr, nullptr);
  if (data) {
    m_saved_data = static_cast<uint8_t>(data & 0xff); //save bottom byte
    uint64_t int3 = 0xcc;
    uint64_t data_with_int3 = ((data & ~0xff) | int3); //set bottom byte to 0xcc
    m_ptrace(PTRACE_POKEDATA, m_pid, m_addr, reinterpret_cast<uint64_t*>(data_with_int3));

    m_enabled = true;
  }
}

void BreakPoint::disable() {
  uint64_t data = m_ptrace(PTRACE_PEEKDATA, m_pid, m_addr, 0);
  uint64_t restored_data = ((data & ~0xff) | m_saved_data);
  m_ptrace(PTRACE_POKEDATA, m_pid, m_addr, reinterpret_cast<uint64_t*>(restored_data));

  m_enabled = false;
}