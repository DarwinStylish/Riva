#include "riva/correlation_resolver.hpp"

#include <algorithm>
#include <map>
#include <string>

namespace riva {

DefaultCorrelationResolver::DefaultCorrelationResolver(CorrelationResolverConfig config)
    : config_(config) {}

std::vector<ResolvedFinding> DefaultCorrelationResolver::Resolve(
    const std::vector<ResolvedFinding>& findings) const {
  std::vector<ResolvedFinding> result;

  // Group by finding ID
  std::map<std::string, std::vector<ResolvedFinding>> groups;
  for (const auto& f : findings) {
    if (f.role == FindingRole::kPrimary) {
      groups[f.finding.id].push_back(f);
    } else {
      result.push_back(f);
    }
  }

  for (auto& [id, group] : groups) {
    // Sort by start time
    std::sort(group.begin(), group.end(), [](const ResolvedFinding& a, const ResolvedFinding& b) {
      return a.finding.time_window_start_us < b.finding.time_window_start_us;
    });

    std::vector<ResolvedFinding> cluster;
    for (const auto& f : group) {
      if (cluster.empty()) {
        cluster.push_back(f);
      } else {
        const std::uint64_t time_diff = f.finding.time_window_start_us - cluster.back().finding.time_window_start_us;
        if (time_diff <= config_.max_cluster_window_us) {
          cluster.push_back(f);
        } else {
          // Process existing cluster
          if (cluster.size() >= config_.min_cluster_size) {
            ResolvedFinding composite = cluster.front();
            composite.finding.title = "Cluster of: " + composite.finding.title;
            composite.finding.time_window_end_us = cluster.back().finding.time_window_end_us;
            composite.resolution_note = "Correlated " + std::to_string(cluster.size()) + " spikes in a tight window.";
            
            // Average confidence
            double total_confidence = 0.0;
            for(const auto& item : cluster) {
              total_confidence += item.finding.confidence;
            }
            composite.finding.confidence = total_confidence / static_cast<double>(cluster.size());
            
            result.push_back(composite);
          } else {
            for (const auto& item : cluster) {
              result.push_back(item);
            }
          }
          cluster.clear();
          cluster.push_back(f);
        }
      }
    }
    // Process final cluster
    if (!cluster.empty()) {
      if (cluster.size() >= config_.min_cluster_size) {
        ResolvedFinding composite = cluster.front();
        composite.finding.title = "Cluster of: " + composite.finding.title;
        composite.finding.time_window_end_us = cluster.back().finding.time_window_end_us;
        composite.resolution_note = "Correlated " + std::to_string(cluster.size()) + " spikes in a tight window.";
        
        double total_confidence = 0.0;
        for(const auto& item : cluster) {
          total_confidence += item.finding.confidence;
        }
        composite.finding.confidence = total_confidence / static_cast<double>(cluster.size());

        result.push_back(composite);
      } else {
        for (const auto& item : cluster) {
          result.push_back(item);
        }
      }
    }
  }

  // Sort final results to match ConfidenceResolver expectations
  std::stable_sort(result.begin(), result.end(), [](const ResolvedFinding& a, const ResolvedFinding& b) {
    if (a.finding.frame_index != b.finding.frame_index) {
      return a.finding.frame_index < b.finding.frame_index;
    }
    if (a.role != b.role) {
      return a.role == FindingRole::kPrimary; // kPrimary (0) comes before kSecondary (1)
    }
    if (std::abs(a.finding.confidence - b.finding.confidence) > 0.0001) {
      return a.finding.confidence > b.finding.confidence;
    }
    return a.finding.id < b.finding.id;
  });

  return result;
}

}  // namespace riva
