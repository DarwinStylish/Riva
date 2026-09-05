#include "riva/default_analysis.hpp"

#include "riva/builtin_signatures.hpp"

namespace riva {

AnalysisEngine CreateDefaultAnalysisEngine(const AnalysisConfig& config) {
  return AnalysisEngine(CreateBuiltinSignatures(), config);
}

}  // namespace riva
