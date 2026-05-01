#include "riva/version.hpp"

namespace riva {

std::string_view Version() noexcept {
  return "0.1.0";
}

std::string_view ProductName() noexcept {
  return "Riva";
}

}  // namespace riva
