#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "riva/normalized_trace.hpp"
#include "riva/status.hpp"

namespace riva {

struct JsonTraceLoaderOptions {
  bool allow_unknown_fields{false};
};

struct JsonTraceLoadResult {
  Status status;
  std::optional<NormalizedTrace> trace;
};

[[nodiscard]] JsonTraceLoadResult LoadNormalizedTraceFromJsonText(
    std::string_view json_text,
    JsonTraceLoaderOptions options = {});

[[nodiscard]] JsonTraceLoadResult LoadNormalizedTraceFromJsonFile(
    const std::string& path,
    JsonTraceLoaderOptions options = {});

}  // namespace riva
