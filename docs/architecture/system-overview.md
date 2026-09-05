# System Overview & Pipeline Architecture

Riva is structured into two main decoupled layers:
1. **`riva_core`**: A pure C++20 static library containing trace models, signature algorithms, resolvers, comparator, budget parser, and reporting engine.
2. **`RivaEditor`**: An Unreal Engine Editor plugin exposing a native Slate UI panel (`SRivaPanel`) and TraceServices loader (`FRivaTraceService`).


## Subsystem Architecture Diagram

```mermaid
graph TD
    subgraph UE_Plugin["Unreal Editor Environment"]
        UI["SRivaPanel (Slate UI)"]
        TS["FRivaTraceService (TraceServices .utrace Loader)"]
        UI -->|Triggers Trace Ingestion| TS
    end

    subgraph CLI_Env["CI / CD & Headless Environment"]
        CLI["riva (Standalone CLI Executable)"]
    end

    subgraph Core_Lib["riva_core (Pure C++20 Static Library)"]
        NT["NormalizedTrace Data Model"]
        AE["AnalysisEngine"]
        TC["TraceComparator"]
        BE["BudgetEvaluator"]
        RE["ReportEngine"]
        TSy["TraceSynthesizer"]
        PS["PerformanceScore & TraceStatistics"]
        
        AE --> NT
        AE --> PS
        TC --> AE
        RE --> AE
        TSy --> NT
    end

    TS -->|Converts .utrace| NT
    CLI -->|Parses JSON Trace| NT
    CLI -->|Executes Budget Check| BE
    CLI -->|Runs Compare Diff| TC
    CLI -->|Generates Markdown/JSON| RE
    UI -->|Displays Findings| RE

    style Core_Lib fill:#1E293B,stroke:#38BDF8,stroke-width:2px,color:#F8FAFC
    style UE_Plugin fill:#0F172A,stroke:#34D399,stroke-width:2px,color:#F8FAFC
    style CLI_Env fill:#0F172A,stroke:#F43F5E,stroke-width:2px,color:#F8FAFC
```

### Strict Decoupling Rules

- **Zero Unreal Headers in Diagnostic Core**: Code under `RivaCore/Public/riva` and the algorithm sources under `RivaCore/Private` rely on standard C++20. Only `RivaCoreModule.cpp` includes the Unreal module bootstrap header.
- **Normalized Data Model**: Native Unreal Insights recordings (`.utrace`) and JSON trace exports are translated into `riva::NormalizedTrace` before diagnostic processing.


## Core Diagnostics Pipeline

When `AnalysisEngine::Analyze(const NormalizedTrace& trace)` is executed, data flows sequentially through 6 pipeline stages:

```mermaid
flowchart TD
    A["NormalizedTrace<br/>(Frames, Durations, Optional Events)"] --> B["1. Rolling Median Spike Detector<br/>(Identifies hitch windows)"]
    B --> C["2. ISignature Rule Engine<br/>(GC, Shader, PSO, IO, RHI, GT/RT, GPU)"]
    C --> D["3. CausalChainResolver<br/>(Relates likely causes & concurrent symptoms)"]
    D --> E["4. ConfidenceResolver<br/>(Scores findings & assigns Primary/Secondary)"]
    E --> F["5. CorrelationResolver<br/>(Clusters repetitive temporal hitches)"]
    F --> S["6. Performance Score & Trace Statistics<br/>(Calculates P95s and 0-100 grade)"]
    S --> G["AnalysisResult<br/>(Resolved findings, hitch count, budget status, score)"]
    G --> H1["FReportEngine<br/>(Markdown / JSON Reports)"]
    G --> H2["ITraceComparator<br/>(Baseline vs. New Diffing)"]

    style A fill:#1F2937,stroke:#9CA3AF,color:#F9FAFB
    style G fill:#064E3B,stroke:#10B981,color:#ECFDF5
    style H1 fill:#1E1B4B,stroke:#6366F1,color:#EEF2FF
    style H2 fill:#1E1B4B,stroke:#6366F1,color:#EEF2FF
```

### Pipeline Stages Explained

1. **SpikeDetector**: Evaluates frame duration metrics using a rolling median window to pinpoint hitch frames and calculate timing windows.
2. **Signature Rules**: Evaluates registered `ISignature` implementations (`STUT_GC`, `STUT_SHADER_COMPILE`, `STUT_PSO_MISS`, etc.) against spike windows and frame event markers.
3. **Causal Graph**: Applies explicit timing and evidence rules to relate possible causes and concurrent symptoms (for example, streaming IO near a Game Thread stall) and adjusts confidence. These are ranked diagnostic hypotheses, not proof of causation.
4. **Confidence Scoring**: Assigns calibrated confidence scores based on evidence weight and selects the `Primary` finding per hitch frame.
5. **Correlation Clustering**: Coalesces repetitive spikes across temporal windows into composite cluster findings.
6. **Performance Scoring**: Evaluates available trace metrics to compute `FTraceStatistics` (P50/P90/P95/P99) and assigns an `FPerformanceScore` (0-100 and A-F grades) from score targets and hitch density. Missing subsystem telemetry is reported as `N/A`; project budget evaluation is a separate gate.


## Evidence Classification Taxonomy

To uphold the "Claim Discipline" of the engine, every piece of evidence attached to a finding is tagged with an `EEvidenceClassification`:

- `OBSERVED`: Direct factual data (e.g., "GPU took 45ms").
- `DERIVED`: Computed metrics (e.g., "P95 exceeded by 10ms").
- `CORRELATED`: Temporally linked events across threads.
- `INFERRED`: Highly likely deductions based on causal chains.
- `SUSPECTED`: Low-confidence hypotheses requiring manual confirmation.
- `RECOMMENDED`: Actionable next steps provided by the rule engine.


## Synthetic Telemetry Generation

The `TraceSynthesizer` creates deterministic controlled fixtures with injected pathologies (for example, a PSO-miss event at frame 60). These fixtures support repeatable rule and UI regression tests. They do not establish real-world diagnostic accuracy; that requires representative captured traces and independently reviewed labels.
