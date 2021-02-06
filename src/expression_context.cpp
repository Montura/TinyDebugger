#include "expression_context.h"

dwarf::taddr ExpressionContext::reg (uint32_t regnum) {
  return getRegisterValueFromDwarfRegister(m_pid, regnum);
}

dwarf::taddr ExpressionContext::pc() {
  user_regs_struct regs {};
  Ptrace::getRegisters(m_pid, &regs);
  return regs.rip- m_load_address;
}

dwarf::taddr ExpressionContext::deref_size (dwarf::taddr address, uint32_t size) {
  return Ptrace::readMemory(m_pid, address + m_load_address);
}
