# ADR 0003: Rank evidence instead of claiming root-cause certainty

Status: Accepted

Date: 2026-09-05

## Context

Performance hitches frequently contain several concurrent symptoms. A long frame can include CPU pressure, rendering work, waits, streaming activity, or garbage collection at the same time. Timing correlation alone does not establish causation.

A diagnostic tool that labels every correlation as a proven root cause would overstate what the telemetry supports.

## Decision

Riva produces evidence-ranked diagnostic findings rather than infallible root-cause claims.

Evidence is classified as observed, derived, correlated, inferred, suspected, or recommended. Relationship and confidence resolvers may prioritize one finding over another, but reports retain supporting evidence and explicit confirmation guidance.

The analysis pipeline must remain deterministic for the same normalized input and configuration.

## Consequences

Reports are suitable for engineering triage while preserving uncertainty.

A primary finding means highest-ranked supported hypothesis in the implemented rule set, not proof of causation.

Signatures should prefer direct telemetry and narrowly defined rules over opaque heuristics.

Real-world accuracy claims require representative captured traces and independently reviewed labels; synthetic fixtures validate deterministic behavior, not production diagnostic accuracy.
