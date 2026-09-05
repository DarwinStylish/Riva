#pragma once

#include <string_view>

#include "riva/export.hpp"

namespace riva {

[[nodiscard]] RIVACORE_API std::string_view Version() noexcept;
[[nodiscard]] RIVACORE_API std::string_view ProductName() noexcept;

}  // namespace riva
