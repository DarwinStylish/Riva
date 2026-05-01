#pragma once

#include <string>

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

[[nodiscard]] std::string ToStableString(SignatureId id);
[[nodiscard]] std::string ToDisplayName(SignatureId id);

}  // namespace riva
