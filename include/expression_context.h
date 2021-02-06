#include "dwarf++.hh"
#include "registers.h"
#include "ptrace_impl.h"

class ExpressionContext : public dwarf::expr_context {
  pid_t m_pid;
  uint64_t m_load_address;
public:
  explicit ExpressionContext (pid_t pid, uint64_t m_load_address) :
    m_pid(pid),
    m_load_address(m_load_address)
  {}

  dwarf::taddr reg (uint32_t regnum) override;
  dwarf::taddr pc() override;
  dwarf::taddr deref_size (dwarf::taddr address, uint32_t size) override;
};
