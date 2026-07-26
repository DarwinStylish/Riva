#include "riva/budget.hpp"
#include "riva/json_utils.hpp"

#include <fstream>
#include <sstream>
#include <string>

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

  auto status = StrictKeys(
      root,
      {"game_thread_ms_max", "render_thread_ms_max", "rhi_thread_ms_max", "gpu_ms_max", "duration_ms_max"},
      "budget",
      false);

  if (!status.ok()) {
    return BudgetLoadResult{status, std::nullopt};
  }

  BudgetConfig config;

  double val;
  auto status_game = ReadOptionalNumber(root, "game_thread_ms_max", val);
  if (!status_game.ok()) return BudgetLoadResult{status_game, std::nullopt};
  if (FindField(root, "game_thread_ms_max")) config.game_thread_ms_max = val;

  auto status_render = ReadOptionalNumber(root, "render_thread_ms_max", val);
  if (!status_render.ok()) return BudgetLoadResult{status_render, std::nullopt};
  if (FindField(root, "render_thread_ms_max")) config.render_thread_ms_max = val;

  auto status_rhi = ReadOptionalNumber(root, "rhi_thread_ms_max", val);
  if (!status_rhi.ok()) return BudgetLoadResult{status_rhi, std::nullopt};
  if (FindField(root, "rhi_thread_ms_max")) config.rhi_thread_ms_max = val;

  auto status_gpu = ReadOptionalNumber(root, "gpu_ms_max", val);
  if (!status_gpu.ok()) return BudgetLoadResult{status_gpu, std::nullopt};
  if (FindField(root, "gpu_ms_max")) config.gpu_ms_max = val;

  auto status_duration = ReadOptionalNumber(root, "duration_ms_max", val);
  if (!status_duration.ok()) return BudgetLoadResult{status_duration, std::nullopt};
  if (FindField(root, "duration_ms_max")) config.duration_ms_max = val;

  return BudgetLoadResult{Status::Ok(), std::move(config)};
}

BudgetLoadResult LoadBudgetConfigFromJsonFile(const std::string& path) {
  std::ifstream file(path);
  if (!file) {
    return BudgetLoadResult{
        Status(StatusCode::kNotFound, "could not open JSON budget file: " + path),
        std::nullopt,
    };
  }

  std::ostringstream buffer;
  buffer << file.rdbuf();

  return LoadBudgetConfigFromJsonText(buffer.str());
}

}  // namespace riva
