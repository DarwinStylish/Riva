#pragma once

#include <string_view>

namespace riva {

[[nodiscard]] std::string_view Version() noexcept;
[[nodiscard]] std::string_view ProductName() noexcept;

}  // namespace riva
