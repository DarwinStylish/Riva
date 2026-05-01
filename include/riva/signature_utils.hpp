#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "riva/normalized_trace.hpp"
#include "riva/spike_detector.hpp"

namespace riva {

struct SpikeContext {
  const Frame* frame{nullptr};
  std::uint64_t window_start_us{0};
  std::uint64_t window_end_us{0};
};

[[nodiscard]] SpikeContext BuildSpikeContext(
    const NormalizedTrace& trace,
    const Spike& spike,
    std::uint64_t padding_us = 2000);

[[nodiscard]] std::vector<const TraceEvent*> EventsInWindow(
    const Frame& frame,
    std::uint64_t window_start_us,
    std::uint64_t window_end_us);

[[nodiscard]] bool ContainsCaseInsensitive(const std::string& haystack, const std::string& needle);

[[nodiscard]] bool EventMatchesAny(
    const TraceEvent& event,
    const std::vector<std::string>& needles);

[[nodiscard]] double ClampConfidence(double value);

[[nodiscard]] std::string FormatMilliseconds(double value);

}  // namespace riva
