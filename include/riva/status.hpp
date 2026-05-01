#pragma once

#include <string>
#include <utility>

namespace riva {

enum class StatusCode {
  kOk = 0,
  kInvalidArgument,
  kNotFound,
  kParseError,
  kInternalError,
};

class Status {
 public:
  Status() = default;

  Status(StatusCode code, std::string message)
      : code_(code), message_(std::move(message)) {}

  [[nodiscard]] static Status Ok() { return Status(); }

  [[nodiscard]] bool ok() const noexcept { return code_ == StatusCode::kOk; }

  [[nodiscard]] StatusCode code() const noexcept { return code_; }

  [[nodiscard]] const std::string& message() const noexcept { return message_; }

 private:
  StatusCode code_{StatusCode::kOk};
  std::string message_{};
};

}  // namespace riva
