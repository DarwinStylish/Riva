#pragma once

#include <string>

#include "riva/export.hpp"
#include "riva/json_trace_loader.hpp"
#include "riva/trace_adapter.hpp"

namespace riva {

class RIVACORE_API JsonTraceAdapter final : public ITraceAdapter {
 public:
  explicit JsonTraceAdapter(const JsonTraceLoaderOptions& options = {});

  [[nodiscard]] std::string name() const override;
  [[nodiscard]] bool SupportsPath(const std::string& path) const override;
  [[nodiscard]] TraceAdapterResult Load(const std::string& path) const override;

 private:
  JsonTraceLoaderOptions options_;
};

}  // namespace riva
