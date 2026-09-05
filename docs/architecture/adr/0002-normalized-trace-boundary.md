# ADR 0002: Normalize all trace sources before analysis

Status: Accepted

Date: 2026-09-05

## Context

Riva accepts structured JSON traces in standalone workflows and is designed to ingest Unreal Insights `.utrace` recordings through TraceServices. These formats expose different providers, field names, metadata, and availability guarantees.

Allowing analysis signatures to understand individual source formats would duplicate parsing logic and make diagnostic behavior source-dependent.

## Decision

Every supported trace source is translated into `riva::NormalizedTrace` before it enters the diagnostic pipeline.

Adapters and Unreal integration code own source-specific parsing and provider handling. The analysis engine, signatures, resolvers, statistics, comparison logic, budget evaluation, and report generation operate only on normalized data.

Unavailable telemetry remains unavailable. It is not synthesized to make downstream analysis appear complete.

## Consequences

Diagnostic behavior is easier to test deterministically.

New trace formats can be added through adapters without rewriting the analysis pipeline.

Source provenance and missing-data semantics must be preserved during normalization.

Native `.utrace` support is limited by the providers that have actually been integrated. The current implementation normalizes frame boundaries and available timing-lane measurements; named native event extraction is not yet implemented.
