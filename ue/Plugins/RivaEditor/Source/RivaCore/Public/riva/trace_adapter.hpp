#pragma once

#include <optional>
#include <string>

#include "riva/normalized_trace.hpp"
#include "riva/status.hpp"

namespace riva {

struct TraceAdapterResult {
  Status status;
  std::optional<NormalizedTrace> trace;
};

class ITraceAdapter {
 public:
  virtual ~ITraceAdapter() = default;

  [[nodiscard]] virtual std::string name() const = 0;
  [[nodiscard]] virtual bool SupportsPath(const std::string& path) const = 0;
  [[nodiscard]] virtual TraceAdapterResult Load(const std::string& path) const = 0;
};

}  // namespace riva
