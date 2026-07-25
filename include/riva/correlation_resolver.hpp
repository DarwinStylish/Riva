#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "riva/analysis_result.hpp"

namespace riva {

struct CorrelationResolverConfig {
  std::uint64_t max_cluster_window_us{5000000}; // 5 seconds
  std::size_t min_cluster_size{3};
};

class ICorrelationResolver {
 public:
  virtual ~ICorrelationResolver() = default;

  [[nodiscard]] virtual std::vector<ResolvedFinding> Resolve(
      const std::vector<ResolvedFinding>& findings) const = 0;
};

class DefaultCorrelationResolver final : public ICorrelationResolver {
 public:
  explicit DefaultCorrelationResolver(CorrelationResolverConfig config = {});

  [[nodiscard]] std::vector<ResolvedFinding> Resolve(
      const std::vector<ResolvedFinding>& findings) const override;

 private:
  CorrelationResolverConfig config_;
};

}  // namespace riva
