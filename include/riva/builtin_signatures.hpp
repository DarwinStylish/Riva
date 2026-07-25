#pragma once

#include <memory>
#include <vector>

#include "riva/signature.hpp"

namespace riva {

[[nodiscard]] std::vector<std::unique_ptr<ISignature>> CreateBuiltinSignatures();

}  // namespace riva
