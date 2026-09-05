#include "riva/spike_detector.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace riva {

double Median(std::vector<double> values) {
  if (values.empty()) {
    return 0.0;
  }

  std::sort(values.begin(), values.end());

  const std::size_t middle = values.size() / 2;
  if ((values.size() % 2) == 1) {
    return values[middle];
  }

  return (values[middle - 1] + values[middle]) / 2.0;
}

RollingMedianSpikeDetector::RollingMedianSpikeDetector(const SpikeDetectionConfig& config)
    : config_(config) {}

const SpikeDetectionConfig& RollingMedianSpikeDetector::config() const noexcept { return config_; }

std::vector<Spike> RollingMedianSpikeDetector::Detect(const NormalizedTrace& trace) const {
  std::vector<Spike> spikes;
  const auto& frames = trace.frames();

  for (std::size_t i = 0; i < frames.size(); ++i) {
    if (i == 0) {
      continue;
    }

    const std::size_t window_begin =
        i > config_.baseline_window_size ? i - config_.baseline_window_size : 0;

    std::vector<double> baseline_values;
    baseline_values.reserve(i - window_begin);

    for (std::size_t j = window_begin; j < i; ++j) {
      baseline_values.push_back(frames[j].duration_ms);
    }

    const double baseline_ms = Median(baseline_values);
    if (baseline_ms <= 0.0) {
      continue;
    }

    const double frame_ms = frames[i].duration_ms;
    const double delta_ms = frame_ms - baseline_ms;
    const double ratio = frame_ms / baseline_ms;

    if (frame_ms >= config_.absolute_threshold_ms && ratio >= config_.ratio_threshold &&
        delta_ms >= config_.minimum_delta_ms) {
      spikes.push_back(Spike{
          frames[i].index,
          frame_ms,
          baseline_ms,
          ratio,
          delta_ms,
      });
    }
  }

  return spikes;
}

}  // namespace riva
