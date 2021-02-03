#include <string>
#include <elf++.hh>
#include <iostream>

#include "symbol.h"

std::string toString(SymbolType st) {
  switch (st) {
    case SymbolType::notype:
      return "notype";
    case SymbolType::object:
      return "object";
    case SymbolType::func:
      return "func";
    case SymbolType::section:
      return "section";
    case SymbolType::file:
      return "file";
  }
  std::cerr << "Unknown SymbolType!" << (int32_t) st << "\n";
  return "";
}

SymbolType toSymbolType(elf::stt sym) {
  switch (sym) {
    case elf::stt::notype:
      return SymbolType::notype;
    case elf::stt::object:
      return SymbolType::object;
    case elf::stt::func:
      return SymbolType::func;
    case elf::stt::section:
      return SymbolType::section;
    case elf::stt::file:
      return SymbolType::file;
    default:
      return SymbolType::notype;
  }
}

std::ostream& operator<<(std::ostream& os, const Symbol& symbol) {
  os << "type: " << toString(symbol.type) << " name: " << symbol.name
    << " addr: 0x" << std::hex << symbol.addr;
  return os;
};