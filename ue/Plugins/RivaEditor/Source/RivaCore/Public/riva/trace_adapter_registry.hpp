#pragma once

#include <memory>
#include <string>
#include <vector>

#include "riva/export.hpp"
#include "riva/trace_adapter.hpp"

namespace riva {

class RIVACORE_API TraceAdapterRegistry {
 public:
  void Register(std::unique_ptr<ITraceAdapter> adapter);

  [[nodiscard]] const ITraceAdapter* FindForPath(const std::string& path) const;
  [[nodiscard]] TraceAdapterResult Load(const std::string& path) const;
  [[nodiscard]] std::size_t size() const noexcept;

 private:
  std::vector<std::unique_ptr<ITraceAdapter>> adapters_;
};

[[nodiscard]] RIVACORE_API TraceAdapterRegistry CreateDefaultTraceAdapterRegistry();

}  // namespace riva
