#include <cstddef>
#include <cstdint>
#include <string_view>

#include "riva/json_trace_loader.hpp"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  const std::string_view input(reinterpret_cast<const char*>(data), size);

  // Normal parser rejection is an expected result. Sanitizers surface memory
  // safety or undefined-behavior failures in this production input boundary.
  static_cast<void>(riva::LoadNormalizedTraceFromJsonText(input));

  return 0;
}
