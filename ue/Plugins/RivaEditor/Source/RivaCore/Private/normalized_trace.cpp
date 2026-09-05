#include "riva/normalized_trace.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <utility>

namespace riva {

NormalizedTrace::NormalizedTrace(std::string source_name) : source_name_(std::move(source_name)) {}

const std::string& NormalizedTrace::source_name() const noexcept { return source_name_; }

const std::vector<Frame>& NormalizedTrace::frames() const noexcept { return frames_; }

bool NormalizedTrace::empty() const noexcept { return frames_.empty(); }

std::size_t NormalizedTrace::frame_count() const noexcept { return frames_.size(); }

Status NormalizedTrace::AddFrame(Frame frame) {
  const double timing_values[] = {
      frame.duration_ms,   frame.game_thread_ms, frame.render_thread_ms,
      frame.rhi_thread_ms, frame.gpu_ms,         frame.physics_ms,
      frame.ai_ms,         frame.network_ms,     frame.loading_ms,
      frame.memory_bytes,
  };
  if (std::any_of(std::begin(timing_values), std::end(timing_values),
                  [](double value) { return !std::isfinite(value) || value < 0.0; })) {
    return Status(StatusCode::kInvalidArgument,
                  "frame timing and memory values must be finite and non-negative");
  }

  const long double duration_us = static_cast<long double>(frame.duration_ms) * 1000.0L;
  const long double remaining_time_us =
      static_cast<long double>(std::numeric_limits<std::uint64_t>::max() - frame.start_time_us);
  if (duration_us > remaining_time_us) {
    return Status(StatusCode::kInvalidArgument, "frame time range exceeds uint64 range");
  }

  if (!frames_.empty() && frame.index <= frames_.back().index) {
    return Status(StatusCode::kInvalidArgument, "frame indices must be strictly increasing");
  }
  if (!frames_.empty() && frame.start_time_us <= frames_.back().start_time_us) {
    return Status(StatusCode::kInvalidArgument, "frame start times must be strictly increasing");
  }

  for (const FTraceCounter& Counter : frame.counters) {
    if (!std::isfinite(Counter.value)) {
      return Status(StatusCode::kInvalidArgument, "frame counter values must be finite");
    }
  }

  frames_.push_back(std::move(frame));
  return Status::Ok();
}

void NormalizedTrace::SetBuildInfo(FBuildInfo InBuildInfo) { build_info_ = std::move(InBuildInfo); }

void NormalizedTrace::SetScenarioInfo(FScenarioInfo InScenarioInfo) {
  scenario_info_ = std::move(InScenarioInfo);
}

const FBuildInfo& NormalizedTrace::build_info() const noexcept { return build_info_; }

const FScenarioInfo& NormalizedTrace::scenario_info() const noexcept { return scenario_info_; }

const std::vector<FTraceThread>& NormalizedTrace::threads() const noexcept { return threads_; }

Status NormalizedTrace::AddThread(FTraceThread InThread) {
  if (!std::isfinite(InThread.utilization) || InThread.utilization < 0.0 ||
      InThread.utilization > 1.0 || !std::isfinite(InThread.total_active_us) ||
      InThread.total_active_us < 0.0 || !std::isfinite(InThread.total_wait_us) ||
      InThread.total_wait_us < 0.0) {
    return Status(StatusCode::kInvalidArgument,
                  "thread utilization and timing values are out of range");
  }

  const auto it = std::find_if(
      threads_.begin(), threads_.end(),
      [&InThread](const FTraceThread& existing) { return existing.id == InThread.id; });

  if (it != threads_.end()) {
    return Status(StatusCode::kInvalidArgument,
                  "duplicate thread id: " + std::to_string(InThread.id));
  }

  threads_.push_back(std::move(InThread));
  return Status::Ok();
}

}  // namespace riva
