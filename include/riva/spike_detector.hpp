#pragma once

#include <cstddef>
#include <vector>

#include "riva/normalized_trace.hpp"

namespace riva {

struct SpikeDetectionConfig {
  double absolute_threshold_ms{33.333};
  double ratio_threshold{1.5};
  double minimum_delta_ms{8.0};
  std::size_t baseline_window_size{5};
};

struct Spike {
  std::size_t frame_index{0};
  double frame_ms{0.0};
  double baseline_ms{0.0};
  double ratio{0.0};
  double delta_ms{0.0};
};

[[nodiscard]] double Median(std::vector<double> values);

class RollingMedianSpikeDetector {
 public:
  explicit RollingMedianSpikeDetector(SpikeDetectionConfig config = {});

  [[nodiscard]] const SpikeDetectionConfig& config() const noexcept;
  [[nodiscard]] std::vector<Spike> Detect(const NormalizedTrace& trace) const;

 private:
  SpikeDetectionConfig config_;
};

}  // namespace riva
