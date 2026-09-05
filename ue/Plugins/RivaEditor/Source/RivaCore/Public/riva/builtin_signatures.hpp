#pragma once

#include <memory>
#include <vector>

#include "riva/export.hpp"
#include "riva/signature.hpp"

namespace riva {

[[nodiscard]] RIVACORE_API std::vector<std::unique_ptr<ISignature>> CreateBuiltinSignatures();

}  // namespace riva
