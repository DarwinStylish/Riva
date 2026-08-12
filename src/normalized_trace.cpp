#include "riva/normalized_trace.hpp"

#include <algorithm>
#include <utility>

namespace riva {

NormalizedTrace::NormalizedTrace(std::string source_name)
    : source_name_(std::move(source_name)) {}

const std::string& NormalizedTrace::source_name() const noexcept {
  return source_name_;
}

const std::vector<Frame>& NormalizedTrace::frames() const noexcept {
  return frames_;
}

bool NormalizedTrace::empty() const noexcept {
  return frames_.empty();
}

std::size_t NormalizedTrace::frame_count() const noexcept {
  return frames_.size();
}

Status NormalizedTrace::AddFrame(Frame frame) {
  if (frame.duration_ms < 0.0) {
    return Status(StatusCode::kInvalidArgument, "frame duration cannot be negative");
  }

  if (!frames_.empty() && frame.index <= frames_.back().index) {
    return Status(StatusCode::kInvalidArgument, "frame indices must be strictly increasing");
  }

  frames_.push_back(std::move(frame));
  return Status::Ok();
}

void NormalizedTrace::SetBuildInfo(FBuildInfo InBuildInfo) {
  build_info_ = std::move(InBuildInfo);
}

void NormalizedTrace::SetScenarioInfo(FScenarioInfo InScenarioInfo) {
  scenario_info_ = std::move(InScenarioInfo);
}

const FBuildInfo& NormalizedTrace::build_info() const noexcept {
  return build_info_;
}

const FScenarioInfo& NormalizedTrace::scenario_info() const noexcept {
  return scenario_info_;
}

const std::vector<FTraceThread>& NormalizedTrace::threads() const noexcept {
  return threads_;
}

Status NormalizedTrace::AddThread(FTraceThread InThread) {
  const auto it = std::find_if(
      threads_.begin(), threads_.end(),
      [&InThread](const FTraceThread& existing) {
        return existing.id == InThread.id;
      });

  if (it != threads_.end()) {
    return Status(StatusCode::kInvalidArgument, "duplicate thread id: " + std::to_string(InThread.id));
  }

  threads_.push_back(std::move(InThread));
  return Status::Ok();
}

}  // namespace riva
