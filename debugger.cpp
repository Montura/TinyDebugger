#include <iostream>
#include <vector>
#include <sstream>
#include "debugger.h"
#include "linenoise/linenoise.h"

template <class Output>
void split(const std::string &s, char delimiter, Output result, int count = 1) {
  std::istringstream iss(s);
  std::string item;
  for (int i = 0; std::getline(iss, item, delimiter) && i < count; ++i) {
    *result++ = item;
  }
}

bool is_prefix(const std::string& s, const std::string& of) {
  if (s.size() > of.size()) {
    return false;
  }
  return std::equal(s.begin(), s.end(), of.begin());
}

void Debugger::run() {
  int wait_status;
  auto options = 0;
  waitpid(m_pid, &wait_status, options);

  char* line = nullptr;
  while ((line = linenoise("minidbg> ")) != nullptr) {
    handle_command(line);
    linenoiseHistoryAdd(line);
    linenoiseFree(line);
  }
}

void Debugger::handle_command(const char* line) {
  std::string command;
  split(line, ' ', &command);

  if (is_prefix(command, "continue")) {
//    continue_execution();
  } else {
    std::cerr << "Unknown command\n";
  }
}