#pragma once

#include <vector>

#include "riva/finding.hpp"

namespace riva {

struct CausalChainResolverConfig {
  bool enabled{true};
};

class ICausalChainResolver {
 public:
  virtual ~ICausalChainResolver() = default;

  [[nodiscard]] virtual std::vector<Finding> Resolve(std::vector<Finding> findings) const = 0;
};

class DefaultCausalChainResolver final : public ICausalChainResolver {
 public:
  explicit DefaultCausalChainResolver(CausalChainResolverConfig config = {});

  [[nodiscard]] std::vector<Finding> Resolve(std::vector<Finding> findings) const override;

 private:
  CausalChainResolverConfig config_;
};

}  // namespace riva
