#pragma once

#include "riva/analysis_engine.hpp"

namespace riva {

[[nodiscard]] AnalysisEngine CreateDefaultAnalysisEngine(AnalysisConfig config = {});

}  // namespace riva
