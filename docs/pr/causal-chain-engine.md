# Release: Causal Chain Engine

## Overview
This PR introduces the **Causal Chain Engine**, the second major pillar of the Studio Diagnostics phase. The engine detects inter-dependencies between concurrent performance bottlenecks and demotes downstream symptoms, pointing users directly to the actual root cause of a hitch instead of bombarding them with cascading failures.

## Core Changes
- **Causal Graph**: `CausalChainResolver` establishes known causality edges, such as `STUT_STREAMING_IO` directly triggering `STUT_CPU_GT` (Game Thread stalls) or `STUT_RHI_SYNC` (GPU wait stalls).
- **Finding Linking**: Diagnosed root causes now possess `related_finding_ids`, referencing the symptomatic spikes they generated.
- **Engine Priority Boost**: When a root cause and a symptom occur within the same frame context, the engine automatically elevates the confidence of the root cause so the `ConfidenceResolver` natively selects it as the primary finding, demoting the symptoms to secondary findings.

## Verification
- Added `riva_causal_chain_resolver_tests` to validate that findings occurring within the same frame correctly establish bidirectional `related_finding_ids` and appropriately alter their confidence weighting.
- Updated `spike_rhi_sync.json` to properly isolate RHI sync waits, verifying that the entire CTest gauntlet runs flawlessly under the new causal rules.
