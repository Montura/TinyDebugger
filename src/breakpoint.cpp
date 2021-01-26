#include "breakpoint.h"
#include "utils.h"

#if __APPLE__
#define PTRACE_PEEKDATA PT_READ_D
#define PTRACE_POKEDATA PT_WRITE_D
#endif

void BreakPoint::enable() {
  uint64_t data = read_memory(m_addr);
  if (data) {
    m_saved_data = static_cast<uint8_t>(data & 0xff); //save bottom byte
    uint64_t int3 = 0xcc;
    uint64_t data_with_int3 = ((data & ~0xff) | int3); //set bottom byte to 0xcc
    write_memory(m_addr, data_with_int3);

    m_enabled = true;
  }
}

void BreakPoint::disable() {
  uint64_t data = read_memory(m_addr);
  uint64_t restored_data = ((data & ~0xff) | m_saved_data);
  write_memory(m_addr, restored_data);

  m_enabled = false;
}

uint64_t BreakPoint::read_memory(uint64_t address) {
  return m_ptrace(PTRACE_PEEKDATA, m_pid, address, nullptr);
}

void BreakPoint::write_memory(uint64_t address, uint64_t value) {
  m_ptrace(PTRACE_POKEDATA, m_pid, address, reinterpret_cast<uint64_t*>(value));
}