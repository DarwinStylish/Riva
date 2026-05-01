#include "riva/normalized_trace.hpp"

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

}  // namespace riva
