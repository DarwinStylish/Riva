#pragma once

#include "riva/analysis_engine.hpp"

namespace riva {

[[nodiscard]] RIVACORE_API AnalysisEngine
CreateDefaultAnalysisEngine(const AnalysisConfig& config = {});

}  // namespace riva
