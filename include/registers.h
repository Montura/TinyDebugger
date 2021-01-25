#pragma once

#include <cstdlib>
#include <string>
#include <array>
#include <sys/user.h>
#include <functional>

constexpr std::size_t n_registers = 27;
enum class Reg : uint64_t {
  rax, rbx, rcx, rdx,
  rdi, rsi, rbp, rsp,
  r8, r9, r10, r11,
  r12, r13, r14, r15,
  rip, rflags, cs,
  orig_rax, fs_base,
  gs_base,
  fs, gs, ss, ds, es
};

// Debug With Arbitrary Record Format (DWARF) debugging format for the AMD64 processor family
// DWARF register numbers:
//  https://www.uclibc.org/docs/psABI-x86_64.pdf, Volume 3.6 DWARF Definition, p. 55
struct RegDescriptor {
  Reg r;
  int32_t dwarf_r;
  std::string_view name;
};

// Figure 3.36: DWARF Register Number
//      Mapping Register Name       Number  Abbreviation
// General Purpose Register  RAX      0         %rax
// General Purpose Register  RDX      1         %rdx
// General Purpose Register  RCX      2         %rcx
// General Purpose Register  RBX      3         %rbx
// General Purpose Register  RSI      4         %rsi
// General Purpose Register  RDI      5         %rdi
// Frame Pointer Register    RBP      6         %rbp
// Stack Pointer Register    RSP      7         %rsp
// Extended Integer Registers 8-15   8-15       %r8–%r15
// Return Address RA                  16
// Vector Registers 0–7              17-24      %xmm0–%xmm7
// Extended Vector Registers 8–15    25-32      %xmm8–%xmm15
// Floating Point Registers 0–7      33-40      %st0–%st7
// MMX Registers 0–7                 41-48      %mm0–%mm7
// Flag Register                      49        %rFLAGS
// Segment Register ES                50        %es
// Segment Register CS                51        %cs
// Segment Register SS                52        %ss
// Segment Register DS                53        %ds
// Segment Register FS                54        %fs
// Segment Register GS                55        %gs
// Reserved                          56-57
// FS Base address                    58        %fs.base
// GS Base address                    59        %gs.base
// Reserved                          60-61
// Task Register                      62        %tr
// LDT Register                       63        %ldtr
// 128-bit Media Control and Status   64        %mxcsr
// x87 Control Word                   65        %fcw
// x87 Status Word                    66        %fsw

// The layout of the registers in the vector is important!
// It must conform user_regs_struct in <sys/user.h>
constexpr std::array<RegDescriptor, n_registers> globalRegisterDescriptors =
    {{
         {Reg::r15, 15, "r15"},
         {Reg::r14, 14, "r14"},
         {Reg::r13, 13, "r13"},
         {Reg::r12, 12, "r12"},
         {Reg::rbp, 6, "rbp"},
         {Reg::rbx, 3, "rbx"},
         {Reg::r11, 11, "r11"},
         {Reg::r10, 10, "r10"},
         {Reg::r9, 9, "r9"},
         {Reg::r8, 8, "r8"},
         {Reg::rax, 0, "rax"},
         {Reg::rcx, 2, "rcx"},
         {Reg::rdx, 1, "rdx"},
         {Reg::rsi, 4, "rsi"},
         {Reg::rdi, 5, "rdi"},
         {Reg::orig_rax, -1, "orig_rax"},
         {Reg::rip, -1, "rip"},
         {Reg::cs, 51, "cs"},
         {Reg::rflags, 49, "eflags"},
         {Reg::rsp, 7, "rsp"},
         {Reg::ss, 52, "ss"},
         {Reg::fs_base, 58, "fs_base"},
         {Reg::gs_base, 59, "gs_base"},
         {Reg::ds, 53, "ds"},
         {Reg::es, 50, "es"},
         {Reg::fs, 54, "fs"},
         {Reg::gs, 55, "gs"}
     }};

typedef std::function<bool(RegDescriptor)> LambdaPredicate;
//  
//  struct Predicate : RegPredicate {
//    const Reg &reg;
//
//    explicit Predicate(const Reg &idx) : reg(idx) {}
//
//    bool operator()(const RegDescriptor &arg) const { return arg.r == reg; }
//  };

uint64_t* getUserRegisterByOffset(user_regs_struct& userRegs, const RegDescriptor* const pos);

const RegDescriptor* findRegDescInGlobalRegisterTable(LambdaPredicate const& predicate);

uint64_t* findRegInUserRegisterTable(pid_t pid, const Reg& r, user_regs_struct& userRegs);

uint64_t getRegisterValue(pid_t pid, Reg const& r);

void setRegisterValue(pid_t pid, Reg const& r, uint64_t value);

uint64_t getRegisterValueFromDwarfRegister(pid_t pid, int regDwarfValue);

std::string_view getRegisterName(Reg r);

Reg getRegisterFromName(const std::string_view& name);

