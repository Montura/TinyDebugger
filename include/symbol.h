#pragma once

#include <ostream>
#include <utility>

enum class SymbolType : int32_t {
  notype,            // No type (e.g., absolute symbol)
  object,            // Data object
  func,              // Function entry point
  section,           // Symbol is associated with a section
  file,              // Source file associated with the
};                   // object file

std::string toString(SymbolType st);
SymbolType toSymbolType(elf::stt sym);

struct Symbol {
  const SymbolType type;
  const std::string name;
  const uint64_t addr;

  explicit Symbol(const elf::Sym<>& data, std::string symbol_name)
  : type(toSymbolType(data.type())),
  name(std::move(symbol_name)),
  addr(data.value)
  {}

  friend std::ostream& operator<<(std::ostream& os, const Symbol& symbol);
};
