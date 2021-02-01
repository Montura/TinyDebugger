#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <sys/user.h>

#include "registers.h"
#include "ptrace_impl.h"

uint64_t* getUserRegisterByOffset(user_regs_struct& userRegs, const RegDescriptor* const pos) {
  return reinterpret_cast<uint64_t*>(&userRegs) + (pos - globalRegisterDescriptors.begin());
}

const RegDescriptor* findRegDescInGlobalRegisterTable(LambdaPredicate const& predicate) {
  auto itBegin = globalRegisterDescriptors.begin();
  auto itEnd = globalRegisterDescriptors.end();

  auto it = std::find_if(itBegin, itEnd, predicate);
  if (it == itEnd) {
    std::cerr << "Out of range in getRegisterValue!\n";
    exit(-1);
  }
  return it;
}

uint64_t* findRegInUserRegisterTable(pid_t pid, const Reg& r, user_regs_struct& userRegs) {
  Ptrace::get_registers(pid, &userRegs);
  const LambdaPredicate& predicate =
      [r](auto&& rd) {
        return rd.r == r;
      };
  auto pRegDescriptor = findRegDescInGlobalRegisterTable(predicate);
  return getUserRegisterByOffset(userRegs, pRegDescriptor);
}

uint64_t getRegisterValue(pid_t pid, Reg const& r) {
  user_regs_struct userRegs{0};
  uint64_t* pUserRegister = findRegInUserRegisterTable(pid, r, userRegs);
  return *(pUserRegister);
}

void setRegisterValue(pid_t pid, Reg const& r, uint64_t value) {
  user_regs_struct userRegs{0};
  uint64_t* pUserRegister = findRegInUserRegisterTable(pid, r, userRegs);

  *pUserRegister = value;
  Ptrace::set_registers(pid, &userRegs);
}

uint64_t getRegisterValueFromDwarfRegister(pid_t pid, int regDwarfValue) {
  const LambdaPredicate& predicate =
      [regDwarfValue](auto&& rd) {
        return rd.dwarf_r == regDwarfValue;
      };
  auto pRegDescriptor = findRegDescInGlobalRegisterTable(predicate);

  return getRegisterValue(pid, pRegDescriptor->r);
}

std::string_view getRegisterName(Reg r) {
  const LambdaPredicate& predicate = [r](auto&& rd) { return rd.r == r; };
  auto it = findRegDescInGlobalRegisterTable(predicate);
  return it->name;
}

Reg getRegisterByName(const std::string_view& name) {
  const LambdaPredicate& predicate = [name](auto&& rd) { return rd.name == name; };
  auto it = findRegDescInGlobalRegisterTable(predicate);
  return it->r;
}
