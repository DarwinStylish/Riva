#pragma once

#include <string>

#include "riva/export.hpp"

namespace riva {

enum class SignatureId {
  kShaderCompile = 0,
  kPsoMiss,
  kStreamingIo,
  kCpuGameThread,
  kCpuRenderThread,
  kRhiSync,
  kGarbageCollection,
  kGpuVarianceLumenVsm,
};

[[nodiscard]] RIVACORE_API std::string ToStableString(SignatureId id);
[[nodiscard]] RIVACORE_API std::string ToDisplayName(SignatureId id);

}  // namespace riva
