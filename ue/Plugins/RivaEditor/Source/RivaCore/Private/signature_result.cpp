#include "riva/signature_result.hpp"

namespace riva {

std::string ToStableString(SignatureId id) {
  switch (id) {
    case SignatureId::kShaderCompile:
      return "STUT_SHADER_COMPILE";
    case SignatureId::kPsoMiss:
      return "STUT_PSO_MISS";
    case SignatureId::kStreamingIo:
      return "STUT_STREAMING_IO";
    case SignatureId::kCpuGameThread:
      return "STUT_CPU_GT";
    case SignatureId::kCpuRenderThread:
      return "STUT_CPU_RT";
    case SignatureId::kRhiSync:
      return "STUT_RHI_SYNC";
    case SignatureId::kGarbageCollection:
      return "STUT_GC";
    case SignatureId::kGpuVarianceLumenVsm:
      return "STUT_GPU_VARIANCE_LUMEN_VSM";
  }

  return "STUT_UNKNOWN";
}

std::string ToDisplayName(SignatureId id) {
  switch (id) {
    case SignatureId::kShaderCompile:
      return "Shader compilation stall";
    case SignatureId::kPsoMiss:
      return "Pipeline state object miss";
    case SignatureId::kStreamingIo:
      return "Streaming or IO stall";
    case SignatureId::kCpuGameThread:
      return "Game thread CPU spike";
    case SignatureId::kCpuRenderThread:
      return "Render thread CPU spike";
    case SignatureId::kRhiSync:
      return "RHI synchronization stall";
    case SignatureId::kGarbageCollection:
      return "Garbage collection stall";
    case SignatureId::kGpuVarianceLumenVsm:
      return "GPU variance from Lumen or virtual shadow maps";
  }

  return "Unknown signature";
}

}  // namespace riva
