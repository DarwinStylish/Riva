#include "riva/causal_chain_resolver.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <utility>

namespace riva {

namespace {

// Maps a likely-cause candidate to findings that commonly occur with it. These
// domain rules affect ranking; temporal co-occurrence does not prove causation.
const std::map<std::string, std::set<std::string>> kFindingRelationships = {
    // GC may occur with Game Thread stalls and RHI sync waits.
    {"STUT_GC", {"STUT_CPU_GT", "STUT_RHI_SYNC"}},

    // IO stalls may occur with Game Thread and RHI sync waits.
    {"STUT_STREAMING_IO", {"STUT_CPU_GT", "STUT_RHI_SYNC"}},

    // Shader compilation may occur with Render Thread stalls and RHI sync waits.
    {"STUT_SHADER_COMPILE", {"STUT_CPU_RT", "STUT_RHI_SYNC"}},

    // A PSO miss may occur with Render Thread stalls and RHI sync waits.
    {"STUT_PSO_MISS", {"STUT_CPU_RT", "STUT_RHI_SYNC"}},

    // GPU variance may occur with RHI sync waits.
    {"STUT_GPU_VARIANCE_LUMEN_VSM", {"STUT_RHI_SYNC"}},

    // An RHI wait may occur with CPU stalls as threads block.
    {"STUT_RHI_SYNC", {"STUT_CPU_GT", "STUT_CPU_RT"}},
};

bool IsLikelyCause(const std::string& candidate_cause, const std::string& candidate_symptom) {
  auto it = kFindingRelationships.find(candidate_cause);
  if (it != kFindingRelationships.end()) {
    return it->second.count(candidate_symptom) > 0;
  }
  return false;
}

}  // namespace

DefaultCausalChainResolver::DefaultCausalChainResolver(CausalChainResolverConfig config)
    : config_(config) {}

std::vector<Finding> DefaultCausalChainResolver::Resolve(std::vector<Finding> findings) const {
  if (!config_.enabled) {
    return findings;
  }

  // We analyze dependencies within the same frame index.
  std::map<std::size_t, std::vector<std::size_t>> frame_to_finding_indices;
  for (std::size_t i = 0; i < findings.size(); ++i) {
    frame_to_finding_indices[findings[i].frame_index].push_back(i);
  }

  for (const auto& [frame_idx, indices] : frame_to_finding_indices) {
    if (indices.size() < 2) continue;  // Need at least 2 findings to form a chain

    for (std::size_t root_idx : indices) {
      for (std::size_t symp_idx : indices) {
        if (root_idx == symp_idx) continue;

        Finding& likely_cause = findings[root_idx];
        Finding& symptom = findings[symp_idx];

        if (IsLikelyCause(likely_cause.id, symptom.id)) {
          // Link them
          if (std::find(likely_cause.related_finding_ids.begin(),
                        likely_cause.related_finding_ids.end(),
                        symptom.id) == likely_cause.related_finding_ids.end()) {
            likely_cause.related_finding_ids.push_back(symptom.id);
          }
          if (std::find(symptom.related_finding_ids.begin(), symptom.related_finding_ids.end(),
                        likely_cause.id) == symptom.related_finding_ids.end()) {
            symptom.related_finding_ids.push_back(likely_cause.id);
          }

          // Raise the configured likely-cause candidate above the concurrent
          // symptom so ConfidenceResolver can preserve the intended rank.
          if (likely_cause.confidence <= symptom.confidence) {
            likely_cause.confidence = std::min(0.95, symptom.confidence + 0.05);
          }
        }
      }
    }
  }

  return findings;
}

}  // namespace riva
