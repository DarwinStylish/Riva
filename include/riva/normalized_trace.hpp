#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "riva/status.hpp"
#include "riva/trace_event.hpp"

namespace riva {

struct Frame {
  std::size_t index{0};
  std::uint64_t start_time_us{0};
  double duration_ms{0.0};
  double game_thread_ms{0.0};
  double render_thread_ms{0.0};
  double rhi_thread_ms{0.0};
  double gpu_ms{0.0};
  std::vector<TraceEvent> events;
};

class NormalizedTrace {
 public:
  explicit NormalizedTrace(std::string source_name);

  [[nodiscard]] const std::string& source_name() const noexcept;
  [[nodiscard]] const std::vector<Frame>& frames() const noexcept;
  [[nodiscard]] bool empty() const noexcept;
  [[nodiscard]] std::size_t frame_count() const noexcept;

  [[nodiscard]] Status AddFrame(Frame frame);

 private:
  std::string source_name_;
  std::vector<Frame> frames_;
};

}  // namespace riva
