#include "riva/json_trace_adapter.hpp"

#include <algorithm>
#include <cctype>
#include <string>

namespace riva {
namespace {

std::string Lowercase(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
    return static_cast<char>(std::tolower(character));
  });
  return value;
}

bool EndsWith(const std::string& value, const std::string& suffix) {
  if (suffix.size() > value.size()) {
    return false;
  }

  return value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

}  // namespace

JsonTraceAdapter::JsonTraceAdapter(const JsonTraceLoaderOptions& options) : options_(options) {}

std::string JsonTraceAdapter::name() const { return "json"; }

bool JsonTraceAdapter::SupportsPath(const std::string& path) const {
  return EndsWith(Lowercase(path), ".json");
}

TraceAdapterResult JsonTraceAdapter::Load(const std::string& path) const {
  auto result = LoadNormalizedTraceFromJsonFile(path, options_);

  if (!result.status.ok()) {
    return TraceAdapterResult{result.status, std::nullopt};
  }
  if (!result.trace.has_value()) {
    return TraceAdapterResult{
        Status(StatusCode::kInternalError, "JSON trace loader succeeded without returning a trace"),
        std::nullopt};
  }

  return TraceAdapterResult{Status::Ok(), std::move(result.trace)};
}

}  // namespace riva
