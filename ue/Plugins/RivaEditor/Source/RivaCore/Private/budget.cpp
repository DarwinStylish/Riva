#include "riva/budget.hpp"

#include <fstream>
#include <sstream>
#include <string>

#include "riva/json_utils.hpp"

namespace riva {

BudgetLoadResult LoadBudgetConfigFromJsonText(std::string_view json_text) {
  JsonValue root;
  JsonParser parser(json_text);

  if (!parser.Parse(root)) {
    return BudgetLoadResult{
        Status(StatusCode::kParseError, parser.error()),
        std::nullopt,
    };
  }

  if (root.type != JsonValue::Type::kObject) {
    return BudgetLoadResult{
        Status(StatusCode::kInvalidArgument, "root must be an object"),
        std::nullopt,
    };
  }

  auto status = StrictKeys(root,
                           {"game_thread_ms_max", "render_thread_ms_max", "rhi_thread_ms_max",
                            "gpu_ms_max", "duration_ms_max"},
                           "budget", false);

  if (!status.ok()) {
    return BudgetLoadResult{status, std::nullopt};
  }

  BudgetConfig config;
  auto read_threshold = [&](const std::string& key, std::optional<double>& destination) -> Status {
    if (FindField(root, key) == nullptr) {
      return Status::Ok();
    }

    double value = 0.0;
    Status read_status = ReadOptionalNumber(root, key, value);
    if (!read_status.ok()) {
      return read_status;
    }
    if (value <= 0.0) {
      return Status(StatusCode::kInvalidArgument,
                    "budget threshold must be greater than zero: " + key);
    }
    destination = value;
    return Status::Ok();
  };

  status = read_threshold("game_thread_ms_max", config.game_thread_ms_max);
  if (!status.ok()) return BudgetLoadResult{status, std::nullopt};
  status = read_threshold("render_thread_ms_max", config.render_thread_ms_max);
  if (!status.ok()) return BudgetLoadResult{status, std::nullopt};
  status = read_threshold("rhi_thread_ms_max", config.rhi_thread_ms_max);
  if (!status.ok()) return BudgetLoadResult{status, std::nullopt};
  status = read_threshold("gpu_ms_max", config.gpu_ms_max);
  if (!status.ok()) return BudgetLoadResult{status, std::nullopt};
  status = read_threshold("duration_ms_max", config.duration_ms_max);
  if (!status.ok()) return BudgetLoadResult{status, std::nullopt};

  if (!config.game_thread_ms_max && !config.render_thread_ms_max && !config.rhi_thread_ms_max &&
      !config.gpu_ms_max && !config.duration_ms_max) {
    return BudgetLoadResult{
        Status(StatusCode::kInvalidArgument, "budget must define at least one threshold"),
        std::nullopt,
    };
  }

  return BudgetLoadResult{Status::Ok(), config};
}

BudgetLoadResult LoadBudgetConfigFromJsonFile(const std::string& path) {
  std::ifstream file(path);
  if (!file) {
    return BudgetLoadResult{
        Status(StatusCode::kNotFound, "could not open JSON budget file: " + path),
        std::nullopt,
    };
  }

  file.seekg(0, std::ios::end);
  const std::streampos FileSize = file.tellg();
  if (FileSize < 0) {
    return BudgetLoadResult{
        Status(StatusCode::kInternalError, "could not determine JSON budget file size: " + path),
        std::nullopt,
    };
  }
  if (static_cast<std::uintmax_t>(static_cast<std::streamoff>(FileSize)) >
      JsonParser::kDefaultMaxInputBytes) {
    return BudgetLoadResult{
        Status(StatusCode::kInvalidArgument, "JSON budget file exceeds configured size limit"),
        std::nullopt,
    };
  }
  file.seekg(0, std::ios::beg);

  std::ostringstream buffer;
  buffer << file.rdbuf();
  if (file.bad()) {
    return BudgetLoadResult{
        Status(StatusCode::kInternalError, "could not read JSON budget file: " + path),
        std::nullopt,
    };
  }

  return LoadBudgetConfigFromJsonText(buffer.str());
}

}  // namespace riva
