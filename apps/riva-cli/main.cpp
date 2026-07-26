#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

#include "riva/analysis_engine.hpp"
#include "riva/builtin_signatures.hpp"
#include "riva/json_trace_adapter.hpp"
#include "riva/report_engine.hpp"
#include "riva/status.hpp"
#include "riva/trace_adapter_registry.hpp"
#include "riva/version.hpp"
#include "riva/budget.hpp"

namespace {

int PrintUsage() {
  std::cout << "Riva " << riva::Version() << "\n"
            << "Deterministic Unreal Engine performance diagnostics companion.\n\n"
            << "Usage:\n"
            << "  riva analyze <trace_file> [options]\n"
            << "  riva check-budget --budget <budget_file> --trace <trace_file>\n"
            << "  riva version\n"
            << "  riva --help\n\n"
            << "Options for analyze:\n"
            << "  --format, -f <markdown|json>   Report output format (default: markdown)\n"
            << "  --output, -o <file_path>       Write report to file instead of standard output\n";
  return 0;
}

int RunAnalyze(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Error: Missing trace file path for 'analyze' command.\n\n";
    PrintUsage();
    return 2;
  }

  std::string trace_path = argv[2];
  if (trace_path == "--help" || trace_path == "-h" || trace_path == "help") {
    return PrintUsage();
  }

  riva::EReportFormat format = riva::EReportFormat::kMarkdown;
  std::string output_path;

  for (int i = 3; i < argc; ++i) {
    const std::string_view arg{argv[i]};
    if (arg == "--format" || arg == "-f") {
      if (i + 1 >= argc) {
        std::cerr << "Error: Missing argument for " << arg << " flag.\n";
        return 2;
      }
      const std::string_view fmt_val{argv[++i]};
      if (fmt_val == "markdown" || fmt_val == "md") {
        format = riva::EReportFormat::kMarkdown;
      } else if (fmt_val == "json") {
        format = riva::EReportFormat::kJson;
      } else {
        std::cerr << "Error: Unsupported report format '" << fmt_val
                  << "'. Use 'markdown' or 'json'.\n";
        return 2;
      }
    } else if (arg == "--output" || arg == "-o") {
      if (i + 1 >= argc) {
        std::cerr << "Error: Missing argument for " << arg << " flag.\n";
        return 2;
      }
      output_path = argv[++i];
    } else {
      std::cerr << "Error: Unknown option '" << arg << "' for analyze command.\n";
      return 2;
    }
  }

  riva::TraceAdapterRegistry registry;
  registry.Register(std::make_unique<riva::JsonTraceAdapter>());

  const auto load_result = registry.Load(trace_path);
  if (!load_result.status.ok()) {
    std::cerr << "Error loading trace '" << trace_path << "': "
              << load_result.status.message() << "\n";
    return 1;
  }

  if (!load_result.trace.has_value()) {
    std::cerr << "Error: Trace adapter returned no data for '" << trace_path << "'.\n";
    return 1;
  }

  riva::AnalysisEngine engine(riva::CreateBuiltinSignatures());
  const auto analysis_result = engine.Analyze(*load_result.trace);

  riva::FReportOptions report_options;
  std::string report_output;
  const auto report_status = riva::FReportEngine::GenerateReport(
      format, analysis_result, report_options, report_output);

  if (!report_status.ok()) {
    std::cerr << "Error generating report: " << report_status.message() << "\n";
    return 1;
  }

  if (!output_path.empty()) {
    std::ofstream out_file(output_path);
    if (!out_file.is_open()) {
      std::cerr << "Error: Could not open output file '" << output_path << "' for writing.\n";
      return 1;
    }
    out_file << report_output;
    out_file.close();
  } else {
    std::cout << report_output;
  }

  return 0;
}

int RunCheckBudget(int argc, char** argv) {
  std::string budget_path;
  std::string trace_path;

  for (int i = 2; i < argc; ++i) {
    const std::string_view arg{argv[i]};
    if (arg == "--budget") {
      if (i + 1 >= argc) {
        std::cerr << "Error: Missing argument for --budget flag.\n";
        return 2;
      }
      budget_path = argv[++i];
    } else if (arg == "--trace") {
      if (i + 1 >= argc) {
        std::cerr << "Error: Missing argument for --trace flag.\n";
        return 2;
      }
      trace_path = argv[++i];
    } else {
      std::cerr << "Error: Unknown option '" << arg << "' for check-budget command.\n";
      return 2;
    }
  }

  if (budget_path.empty() || trace_path.empty()) {
    std::cerr << "Error: --budget and --trace are required for check-budget command.\n\n";
    PrintUsage();
    return 2;
  }

  auto budget_result = riva::LoadBudgetConfigFromJsonFile(budget_path);
  if (!budget_result.status.ok()) {
    std::cerr << "Error loading budget '" << budget_path << "': " << budget_result.status.message() << "\n";
    return 1;
  }

  riva::TraceAdapterRegistry registry;
  registry.Register(std::make_unique<riva::JsonTraceAdapter>());

  const auto load_result = registry.Load(trace_path);
  if (!load_result.status.ok()) {
    std::cerr << "Error loading trace '" << trace_path << "': " << load_result.status.message() << "\n";
    return 1;
  }

  if (!load_result.trace.has_value()) {
    std::cerr << "Error: Trace adapter returned no data for '" << trace_path << "'.\n";
    return 1;
  }

  riva::AnalysisConfig config;
  config.budget = budget_result.config;

  riva::AnalysisEngine engine(riva::CreateBuiltinSignatures(), std::move(config));
  const auto analysis_result = engine.Analyze(*load_result.trace);

  if (analysis_result.budget_status.breached) {
    std::cerr << "Budget check failed! The following metrics exceeded their budget thresholds:\n";
    for (const auto& metric : analysis_result.budget_status.breached_metrics) {
      std::cerr << "  - " << metric << "\n";
    }
    return 3;
  }

  std::cout << "Budget check passed.\n";
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc <= 1) {
    return PrintUsage();
  }

  const std::string_view command{argv[1]};

  if (command == "version" || command == "--version" || command == "-v") {
    std::cout << riva::ProductName() << " " << riva::Version() << "\n";
    return 0;
  }

  if (command == "--help" || command == "-h" || command == "help") {
    return PrintUsage();
  }

  if (command == "analyze") {
    return RunAnalyze(argc, argv);
  }

  if (command == "check-budget") {
    return RunCheckBudget(argc, argv);
  }

  std::cerr << "Unknown command: " << command << "\n\n";
  PrintUsage();
  return 2;
}
