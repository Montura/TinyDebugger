#include "breakpoint.h"
#include "ptrace_impl.h"

#if __APPLE__
#define PTRACE_PEEKDATA PT_READ_D
#define PTRACE_POKEDATA PT_WRITE_D
#endif

void BreakPoint::enable() {
  uint64_t data = Ptrace::readMemory(m_pid, m_addr);
  if (data) {
    m_saved_data = static_cast<uint8_t>(data & 0xff); // save the first byte
    uint64_t int3 = 0xcc;
    uint64_t data_with_int3 = ((data & ~0xff) | int3); // zeros the first byte and set it to 0xcc
    Ptrace::writeMemory(m_pid, m_addr, data_with_int3);

    m_enabled = true;
  }
}

void BreakPoint::disable() {
  uint64_t data = Ptrace::readMemory(m_pid, m_addr);
  uint64_t restored_data = ((data & ~0xff) | m_saved_data); // zeros the first byte and restore it to the initial value
  Ptrace::writeMemory(m_pid, m_addr, restored_data);

  m_enabled = false;
}