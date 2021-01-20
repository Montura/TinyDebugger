#pragma once


#include <string>
#include <utility>

class Debugger {
  std::string m_prog_name;
  pid_t m_pid;

public:
  Debugger(std::string prog_name, pid_t pid): m_prog_name{std::move(prog_name)}, m_pid{pid} {}

  void run();
  void handle_command(const char* command);

};


