#pragma once

#include <string>

#include "riva/json_trace_loader.hpp"
#include "riva/trace_adapter.hpp"

namespace riva {

class JsonTraceAdapter final : public ITraceAdapter {
 public:
  explicit JsonTraceAdapter(JsonTraceLoaderOptions options = {});

  [[nodiscard]] std::string name() const override;
  [[nodiscard]] bool SupportsPath(const std::string& path) const override;
  [[nodiscard]] TraceAdapterResult Load(const std::string& path) const override;

 private:
  JsonTraceLoaderOptions options_;
};

}  // namespace riva
