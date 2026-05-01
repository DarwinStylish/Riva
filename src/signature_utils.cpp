#include "riva/signature_utils.hpp"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>

namespace riva {
namespace {

std::string Lowercase(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
    return static_cast<char>(std::tolower(character));
  });
  return value;
}

}  // namespace

SpikeContext BuildSpikeContext(
    const NormalizedTrace& trace,
    const Spike& spike,
    std::uint64_t padding_us) {
  SpikeContext context;

  if (spike.frame_index >= trace.frames().size()) {
    return context;
  }

  context.frame = &trace.frames()[spike.frame_index];

  const std::uint64_t frame_start = context.frame->start_time_us;
  const std::uint64_t frame_duration_us =
      static_cast<std::uint64_t>(context.frame->duration_ms * 1000.0);
  const std::uint64_t frame_end = frame_start + frame_duration_us;

  context.window_start_us = frame_start > padding_us ? frame_start - padding_us : 0;
  context.window_end_us = frame_end + padding_us;

  return context;
}

std::vector<const TraceEvent*> EventsInWindow(
    const Frame& frame,
    std::uint64_t window_start_us,
    std::uint64_t window_end_us) {
  std::vector<const TraceEvent*> events;

  for (const auto& event : frame.events) {
    const bool overlaps_window =
        event.start_time_us <= window_end_us && event.EndTimeUs() >= window_start_us;

    if (overlaps_window) {
      events.push_back(&event);
    }
  }

  return events;
}

bool ContainsCaseInsensitive(const std::string& haystack, const std::string& needle) {
  if (needle.empty()) {
    return true;
  }

  return Lowercase(haystack).find(Lowercase(needle)) != std::string::npos;
}

bool EventMatchesAny(const TraceEvent& event, const std::vector<std::string>& needles) {
  const std::string combined =
      event.name + " " + event.category + " " + event.thread_name;

  for (const auto& needle : needles) {
    if (ContainsCaseInsensitive(combined, needle)) {
      return true;
    }
  }

  return false;
}

double ClampConfidence(double value) {
  if (value < 0.0) {
    return 0.0;
  }

  if (value > 1.0) {
    return 1.0;
  }

  return value;
}

std::string FormatMilliseconds(double value) {
  std::ostringstream stream;
  stream << std::fixed << std::setprecision(2) << value << " ms";
  return stream.str();
}

}  // namespace riva
