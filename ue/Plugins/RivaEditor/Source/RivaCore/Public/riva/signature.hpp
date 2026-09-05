#pragma once

#include <string>
#include <vector>

#include "riva/finding.hpp"
#include "riva/normalized_trace.hpp"
#include "riva/spike_detector.hpp"

namespace riva {

class ISignature {
 public:
  virtual ~ISignature() = default;

  [[nodiscard]] virtual std::string id() const = 0;
  [[nodiscard]] virtual std::string name() const = 0;

  [[nodiscard]] virtual std::vector<Finding> Analyze(
      const NormalizedTrace& trace,
      const std::vector<Spike>& spikes) const = 0;
};

}  // namespace riva
