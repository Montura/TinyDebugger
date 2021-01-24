#include "breakpoint.h"
#include "utils.h"

template<class T>
void BreakPoint<T>::enable() {
  auto data = m_ptrace(PTRACE_PEEKDATA, m_pid, m_addr, 0);
  if (data) {
    m_saved_data = static_cast<uint8_t>(data & 0xff); //save bottom byte
    uint64_t int3 = 0xcc;
    uint64_t data_with_int3 = ((data & ~0xff) | int3); //set bottom byte to 0xcc
    m_ptrace(PTRACE_POKEDATA, m_pid, m_addr, data_with_int3);

    m_enabled = true;
  }
}

template<class T>
void BreakPoint<T>::disable() {
  auto data = m_ptrace(PTRACE_PEEKDATA, m_pid, m_addr, 0);
  auto restored_data = ((data & ~0xff) | m_saved_data);
  m_ptrace(PTRACE_POKEDATA, m_pid, m_addr, restored_data);

  m_enabled = false;
}