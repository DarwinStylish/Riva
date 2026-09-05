#pragma once

#include <optional>
#include <string>
#include <vector>

#include "riva/export.hpp"
#include "riva/status.hpp"

namespace riva {

struct BudgetConfig {
  std::optional<double> game_thread_ms_max;
  std::optional<double> render_thread_ms_max;
  std::optional<double> rhi_thread_ms_max;
  std::optional<double> gpu_ms_max;
  std::optional<double> duration_ms_max;
};

// Loads a budget configuration from a JSON file.
// Schema expects an object with optional number fields:
// {
//   "game_thread_ms_max": 16.6,
//   "render_thread_ms_max": 16.6,
//   "rhi_thread_ms_max": 16.6,
//   "gpu_ms_max": 16.6,
//   "duration_ms_max": 33.3
// }
struct BudgetLoadResult {
  Status status;
  std::optional<BudgetConfig> config;
};

RIVACORE_API BudgetLoadResult LoadBudgetConfigFromJsonFile(const std::string& path);

RIVACORE_API BudgetLoadResult LoadBudgetConfigFromJsonText(std::string_view json_text);

}  // namespace riva
