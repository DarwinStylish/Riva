#include <cstdlib>
#include <iostream>
#include <string_view>

#include "riva/status.hpp"
#include "riva/version.hpp"

namespace {

void Expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAILED: " << message << "\n";
    std::exit(1);
  }
}

}  // namespace

int main() {
  Expect(riva::ProductName() == std::string_view{"Riva"}, "product name must be Riva");
  Expect(!riva::Version().empty(), "version must not be empty");

  const riva::Status ok = riva::Status::Ok();
  Expect(ok.ok(), "default OK status must be ok");

  const riva::Status error{
      riva::StatusCode::kInvalidArgument,
      "invalid input",
  };

  Expect(!error.ok(), "invalid argument status must not be ok");
  Expect(error.code() == riva::StatusCode::kInvalidArgument, "status code must be preserved");
  Expect(error.message() == "invalid input", "status message must be preserved");

  return 0;
}
