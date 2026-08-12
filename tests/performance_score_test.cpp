#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

#include "riva/performance_score.hpp"
#include "riva/trace_statistics.hpp"

namespace {

void Expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAILED: " << message << "\n";
    std::exit(1);
  }
}

bool InRange(double value, double low, double high) {
  return value >= low && value <= high;
}

riva::FTraceStatistics MakeCleanStats() {
  riva::FTraceStatistics stats;
  stats.total_frames = 100;
  stats.p50_ms = 14.0;
  stats.p90_ms = 15.0;
  stats.p95_ms = 15.5;
  stats.p99_ms = 16.0;
  stats.min_ms = 13.0;
  stats.max_ms = 16.0;
  stats.mean_ms = 14.2;
  stats.hitch_count = 0;
  stats.hitch_percentage = 0.0;
  stats.game_thread_p95_ms = 9.0;
  stats.render_thread_p95_ms = 7.5;
  stats.gpu_p95_ms = 10.0;
  return stats;
}

riva::FTraceStatistics MakeModerateStats() {
  riva::FTraceStatistics stats;
  stats.total_frames = 100;
  stats.p50_ms = 16.0;
  stats.p90_ms = 22.0;
  stats.p95_ms = 25.0;
  stats.p99_ms = 35.0;
  stats.min_ms = 14.0;
  stats.max_ms = 48.0;
  stats.mean_ms = 17.5;
  stats.hitch_count = 5;
  stats.hitch_percentage = 5.0;
  stats.game_thread_p95_ms = 18.0;
  stats.render_thread_p95_ms = 12.0;
  stats.gpu_p95_ms = 14.0;
  return stats;
}

riva::FTraceStatistics MakeTerribleStats() {
  riva::FTraceStatistics stats;
  stats.total_frames = 100;
  stats.p50_ms = 28.0;
  stats.p90_ms = 50.0;
  stats.p95_ms = 60.0;
  stats.p99_ms = 85.0;
  stats.min_ms = 20.0;
  stats.max_ms = 120.0;
  stats.mean_ms = 35.0;
  stats.hitch_count = 20;
  stats.hitch_percentage = 20.0;
  stats.game_thread_p95_ms = 40.0;
  stats.render_thread_p95_ms = 30.0;
  stats.gpu_p95_ms = 45.0;
  return stats;
}

void TestCleanTraceGetsA() {
  auto stats = MakeCleanStats();
  auto score = riva::ComputePerformanceScore(stats);

  Expect(score.overall >= 90.0, "clean trace must score >= 90");
  Expect(score.overall_grade == "A", "clean trace must be grade A");
  Expect(!score.subsystems.empty(), "subsystems must be populated");

  for (const auto& sub : score.subsystems) {
    Expect(sub.score >= 80.0, ("clean subsystem " + sub.name + " must score >= 80").c_str());
  }
}

void TestModerateTraceGetsCorD() {
  auto stats = MakeModerateStats();
  auto score = riva::ComputePerformanceScore(stats);

  Expect(InRange(score.overall, 40.0, 79.0),
         "moderate trace must score between 40-79");
  Expect(score.overall_grade == "C" || score.overall_grade == "D" || score.overall_grade == "F",
         "moderate trace must be grade C, D, or F");
}

void TestTerribleTraceGetsF() {
  auto stats = MakeTerribleStats();
  auto score = riva::ComputePerformanceScore(stats);

  Expect(score.overall < 60.0, "terrible trace must score < 60");
  Expect(score.overall_grade == "F" || score.overall_grade == "D",
         "terrible trace must be grade F or D");
}

void TestEmptyTraceGetsFullScore() {
  riva::FTraceStatistics stats;  // total_frames = 0
  auto score = riva::ComputePerformanceScore(stats);

  Expect(score.overall == 100.0, "empty trace must score 100");
  Expect(score.overall_grade == "A", "empty trace must be grade A");
}

void TestSubsystemScoresReflectMetrics() {
  auto stats = MakeCleanStats();
  stats.game_thread_p95_ms = 25.0;  // Over budget
  stats.render_thread_p95_ms = 7.0;  // Within budget
  stats.gpu_p95_ms = 10.0;  // Within budget

  auto score = riva::ComputePerformanceScore(stats);

  // Find game thread subsystem — it should be penalized
  bool found_game = false;
  bool found_render = false;
  for (const auto& sub : score.subsystems) {
    if (sub.name == "Game Thread") {
      found_game = true;
      Expect(sub.score < 90.0, "game thread with P95=25ms must be penalized");
      Expect(!sub.deductions.empty(), "game thread must have deductions");
    }
    if (sub.name == "Render Thread") {
      found_render = true;
      Expect(sub.score >= 90.0, "render thread with P95=7ms must score well");
    }
  }
  Expect(found_game, "game thread subsystem must exist");
  Expect(found_render, "render thread subsystem must exist");
}

void TestScoreConfigOverrides() {
  auto stats = MakeCleanStats();
  stats.p95_ms = 30.0;

  // With default config (target 16.67ms), this should penalize
  auto score_default = riva::ComputePerformanceScore(stats);

  // With relaxed config (target 33.33ms), this should be fine
  riva::FScoreConfig relaxed;
  relaxed.target_frame_ms = 33.333;
  auto score_relaxed = riva::ComputePerformanceScore(stats, relaxed);

  Expect(score_relaxed.overall > score_default.overall,
         "relaxed config must produce higher score than default for P95=30ms");
}

void TestGradeMapping() {
  // Verify grade boundaries by constructing stats that land in specific ranges
  auto stats_a = MakeCleanStats();
  auto score_a = riva::ComputePerformanceScore(stats_a);
  Expect(score_a.overall_grade == "A", "grade A boundary check");

  // No direct way to force exact scores, but we can verify the grade function
  // is consistent with what we get
  if (score_a.overall >= 90.0) {
    Expect(score_a.overall_grade == "A", "90+ must be A");
  }
}

}  // namespace

int main() {
  TestCleanTraceGetsA();
  TestModerateTraceGetsCorD();
  TestTerribleTraceGetsF();
  TestEmptyTraceGetsFullScore();
  TestSubsystemScoresReflectMetrics();
  TestScoreConfigOverrides();
  TestGradeMapping();
  std::cout << "All performance score tests passed successfully!\n";
  return 0;
}
