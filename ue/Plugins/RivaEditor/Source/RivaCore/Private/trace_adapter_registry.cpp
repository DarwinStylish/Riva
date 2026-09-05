#include "riva/trace_adapter_registry.hpp"

#include <memory>
#include <utility>

#include "riva/json_trace_adapter.hpp"

namespace riva {

void TraceAdapterRegistry::Register(std::unique_ptr<ITraceAdapter> adapter) {
  if (adapter) {
    adapters_.push_back(std::move(adapter));
  }
}

const ITraceAdapter* TraceAdapterRegistry::FindForPath(const std::string& path) const {
  for (const auto& adapter : adapters_) {
    if (adapter && adapter->SupportsPath(path)) {
      return adapter.get();
    }
  }

  return nullptr;
}

TraceAdapterResult TraceAdapterRegistry::Load(const std::string& path) const {
  const ITraceAdapter* adapter = FindForPath(path);

  if (adapter == nullptr) {
    return TraceAdapterResult{
        Status(StatusCode::kInvalidArgument, "no trace adapter supports path: " + path),
        std::nullopt,
    };
  }

  return adapter->Load(path);
}

std::size_t TraceAdapterRegistry::size() const noexcept {
  return adapters_.size();
}

TraceAdapterRegistry CreateDefaultTraceAdapterRegistry() {
  TraceAdapterRegistry registry;
  registry.Register(std::make_unique<JsonTraceAdapter>());
  return registry;
}

}  // namespace riva
