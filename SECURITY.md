# Security Policy

## Supported versions

Riva is currently an early-stage project. Security fixes are applied to the latest development line rather than maintained across a long-term support matrix.

## Reporting a vulnerability

Please do not open a public issue for a vulnerability that could enable code execution, memory corruption, unsafe file handling, or exposure of sensitive trace data.

Report security issues privately through GitHub Security Advisories when available for this repository. Include:

- affected revision or release;
- operating system and compiler or Unreal Engine version;
- minimal reproduction steps;
- relevant input files or traces, if safe to share;
- sanitizer or crash output, when applicable;
- your assessment of impact.

Do not include secrets, proprietary game content, or confidential trace data in public reports.

## Scope

Security-sensitive surfaces include the JSON trace parser, native trace ingestion boundaries, report/file output paths, fuzzing and replay tooling, and Unreal Editor file handling.

Normal rejection of malformed input is not itself a security vulnerability. Reports are most useful when they demonstrate an unintended safety, integrity, or availability impact.
