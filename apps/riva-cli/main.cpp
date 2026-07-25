#include <iostream>
#include <string_view>

#include "riva/version.hpp"

namespace {

int PrintUsage() {
  std::cout << "Riva " << riva::Version() << "\n"
            << "Deterministic Unreal Engine performance diagnostics companion.\n\n"
            << "Usage:\n"
            << "  riva version\n"
            << "  riva --help\n";
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc <= 1) {
    return PrintUsage();
  }

  const std::string_view command{argv[1]};

  if (command == "version" || command == "--version" || command == "-v") {
    std::cout << riva::ProductName() << " " << riva::Version() << "\n";
    return 0;
  }

  if (command == "--help" || command == "-h" || command == "help") {
    return PrintUsage();
  }

  std::cerr << "Unknown command: " << command << "\n\n";
  PrintUsage();
  return 2;
}
