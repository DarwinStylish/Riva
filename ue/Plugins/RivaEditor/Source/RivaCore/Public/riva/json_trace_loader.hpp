#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "riva/export.hpp"
#include "riva/json_utils.hpp"
#include "riva/normalized_trace.hpp"
#include "riva/status.hpp"

namespace riva {

struct JsonTraceLoaderOptions {
  bool allow_unknown_fields{false};
  std::size_t max_input_bytes{JsonParser::kDefaultMaxInputBytes};
  std::size_t max_nesting_depth{JsonParser::kDefaultMaxNestingDepth};
};

struct JsonTraceLoadResult {
  Status status;
  std::optional<NormalizedTrace> trace;
};

[[nodiscard]] RIVACORE_API JsonTraceLoadResult LoadNormalizedTraceFromJsonText(
    std::string_view json_text, const JsonTraceLoaderOptions& options = {});

[[nodiscard]] RIVACORE_API JsonTraceLoadResult LoadNormalizedTraceFromJsonFile(
    const std::string& path, const JsonTraceLoaderOptions& options = {});

}  // namespace riva
