#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <sys/user.h>

#include "registers.h"
#include "ptrace_impl.h"

const RegDescriptor* findRegisterDescription(LambdaPredicate const& predicate) {
  auto itBegin = GLOBAL_REGISTER_DESC_TABLE.begin();
  auto itEnd = GLOBAL_REGISTER_DESC_TABLE.end();

  auto it = std::find_if(itBegin, itEnd, predicate);
  if (it == itEnd) {
    std::cerr << "Out of range in getRegisterValue!\n";
    exit(-1);
  }
  return it;
}

uint64_t* findRegister(pid_t pid, const Reg& r, user_regs_struct& userRegs) {
  Ptrace::getRegisters(pid, &userRegs);
  const LambdaPredicate& predicate =
      [r](auto&& rd) {
        return rd.r == r;
      };
  auto it = findRegisterDescription(predicate);
  // The cast to uint64_t is safe because user_regs_struct is a standard layout type
  // Normal way -> to write switch for RegDescriptor value and return proper filed for userRegs
  return reinterpret_cast<uint64_t*>(&userRegs) + (it - GLOBAL_REGISTER_DESC_TABLE.begin());
}

uint64_t getRegisterValue(pid_t pid, Reg const& r) {
  user_regs_struct userRegs{0};
  uint64_t* pUserRegister = findRegister(pid, r, userRegs);
  return *(pUserRegister);
}

void setRegisterValue(pid_t pid, Reg const& r, uint64_t value) {
  user_regs_struct userRegs{0};
  uint64_t* pUserRegister = findRegister(pid, r, userRegs);

  *pUserRegister = value;
  Ptrace::setRegisters(pid, &userRegs);
}

uint64_t getRegisterValueFromDwarfRegister(pid_t pid, int dwarfRNum) {
  const LambdaPredicate& predicate =
      [dwarfRNum](auto&& rd) {
        return rd.dwarf_r == dwarfRNum;
      };
  auto it = findRegisterDescription(predicate);
  return getRegisterValue(pid, it->r);
}

std::string_view getRegisterName(Reg r) {
  const LambdaPredicate& predicate = [r](auto&& rd) { return rd.r == r; };
  auto it = findRegisterDescription(predicate);
  return it->name;
}

Reg getRegisterByName(const std::string_view& name) {
  const LambdaPredicate& predicate = [name](auto&& rd) { return rd.name == name; };
  auto it = findRegisterDescription(predicate);
  return it->r;
}
