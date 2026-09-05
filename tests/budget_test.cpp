#include "riva/budget.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {
void Expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAILED: " << message << "\n";
    std::exit(1);
  }
}
void ExpectDoubleEq(double a, double b, const char* message) {
  if (std::abs(a - b) > 0.0001) {
    std::cerr << "FAILED: " << message << " (expected " << a << ", got " << b << ")\n";
    std::exit(1);
  }
}
}  // namespace

void TestLoadValidBudgetJson() {
  const std::string json = R"({
    "game_thread_ms_max": 16.6,
    "render_thread_ms_max": 16.6,
    "rhi_thread_ms_max": 16.6,
    "gpu_ms_max": 16.6,
    "duration_ms_max": 33.3
  })";

  auto result = riva::LoadBudgetConfigFromJsonText(json);
  Expect(result.status.ok(), "Valid JSON should return ok status");
  Expect(result.config.has_value(), "Valid JSON should parse config");

  ExpectDoubleEq(*result.config->game_thread_ms_max, 16.6, "game_thread_ms_max");
  ExpectDoubleEq(*result.config->render_thread_ms_max, 16.6, "render_thread_ms_max");
  ExpectDoubleEq(*result.config->rhi_thread_ms_max, 16.6, "rhi_thread_ms_max");
  ExpectDoubleEq(*result.config->gpu_ms_max, 16.6, "gpu_ms_max");
  ExpectDoubleEq(*result.config->duration_ms_max, 33.3, "duration_ms_max");
}

void TestLoadPartialBudgetJson() {
  const std::string json = R"({
    "game_thread_ms_max": 16.6,
    "gpu_ms_max": 33.3
  })";

  auto result = riva::LoadBudgetConfigFromJsonText(json);
  Expect(result.status.ok(), "Partial JSON should return ok status");
  Expect(result.config.has_value(), "Partial JSON should parse config");

  ExpectDoubleEq(*result.config->game_thread_ms_max, 16.6, "game_thread_ms_max partial");
  ExpectDoubleEq(*result.config->gpu_ms_max, 33.3, "gpu_ms_max partial");
  Expect(!result.config->render_thread_ms_max.has_value(),
         "render_thread_ms_max should be nullopt");
  Expect(!result.config->rhi_thread_ms_max.has_value(), "rhi_thread_ms_max should be nullopt");
  Expect(!result.config->duration_ms_max.has_value(), "duration_ms_max should be nullopt");
}

void TestInvalidBudgetJson() {
  const std::string json = R"({
    "game_thread_ms_max": "invalid"
  })";

  auto result = riva::LoadBudgetConfigFromJsonText(json);
  Expect(!result.status.ok(), "Invalid JSON value should fail");
  Expect(!result.config.has_value(), "Invalid JSON value should not parse config");
}

void TestRejectsEmptyAndNonPositiveBudgets() {
  auto result = riva::LoadBudgetConfigFromJsonText("{}");
  Expect(!result.status.ok(), "Empty budget should not create a vacuous passing gate");
  Expect(!result.config.has_value(), "Empty budget should not produce a config");

  result = riva::LoadBudgetConfigFromJsonText(R"({"duration_ms_max": 0})");
  Expect(!result.status.ok(), "Zero budget threshold should fail");

  result = riva::LoadBudgetConfigFromJsonText(R"({"gpu_ms_max": -1})");
  Expect(!result.status.ok(), "Negative budget threshold should fail");
}

int main() {
  TestLoadValidBudgetJson();
  TestLoadPartialBudgetJson();
  TestInvalidBudgetJson();
  TestRejectsEmptyAndNonPositiveBudgets();
  std::cout << "All budget tests passed!\n";
  return 0;
}
