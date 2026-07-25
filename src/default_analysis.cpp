#include "riva/default_analysis.hpp"

#include "riva/builtin_signatures.hpp"

namespace riva {

AnalysisEngine CreateDefaultAnalysisEngine(AnalysisConfig config) {
  return AnalysisEngine(CreateBuiltinSignatures(), config);
}

}  // namespace riva
