#include "riva/causal_chain_resolver.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <utility>

namespace riva {

namespace {

// Maps a root cause signature ID to a set of its known symptomatic signature IDs.
const std::map<std::string, std::set<std::string>> kCausalDependencies = {
    // GC causes Game Thread stalls and RHI sync waits
    {"STUT_GC", {"STUT_CPU_GT", "STUT_RHI_SYNC"}},
    
    // IO stalling causes Game Thread and RHI sync waits
    {"STUT_STREAMING_IO", {"STUT_CPU_GT", "STUT_RHI_SYNC"}},
    
    // Shader compilation causes Render Thread stalls and RHI sync waits
    {"STUT_SHADER_COMPILE", {"STUT_CPU_RT", "STUT_RHI_SYNC"}},
    
    // PSO miss causes Render Thread stalls and RHI sync waits
    {"STUT_PSO_MISS", {"STUT_CPU_RT", "STUT_RHI_SYNC"}},
    
    // GPU variance causes RHI sync waits
    {"STUT_GPU_VARIANCE_LUMEN_VSM", {"STUT_RHI_SYNC"}},
    
    // RHI Sync (GPU Wait) causes CPU stalls as the threads block
    {"STUT_RHI_SYNC", {"STUT_CPU_GT", "STUT_CPU_RT"}},
};

bool IsRootCause(const std::string& candidate_root, const std::string& candidate_symptom) {
  auto it = kCausalDependencies.find(candidate_root);
  if (it != kCausalDependencies.end()) {
    return it->second.count(candidate_symptom) > 0;
  }
  return false;
}

} // namespace

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
    if (indices.size() < 2) continue; // Need at least 2 findings to form a chain

    for (std::size_t root_idx : indices) {
      for (std::size_t symp_idx : indices) {
        if (root_idx == symp_idx) continue;

        Finding& root = findings[root_idx];
        Finding& symptom = findings[symp_idx];

        if (IsRootCause(root.id, symptom.id)) {
          // Link them
          if (std::find(root.related_finding_ids.begin(), root.related_finding_ids.end(), symptom.id) == root.related_finding_ids.end()) {
            root.related_finding_ids.push_back(symptom.id);
          }
          if (std::find(symptom.related_finding_ids.begin(), symptom.related_finding_ids.end(), root.id) == symptom.related_finding_ids.end()) {
            symptom.related_finding_ids.push_back(root.id);
          }
          
          // Elevate the root cause confidence over the symptom so ConfidenceResolver picks it
          if (root.confidence <= symptom.confidence) {
            root.confidence = std::min(0.95, symptom.confidence + 0.05);
          }
        }
      }
    }
  }

  return findings;
}

}  // namespace riva
