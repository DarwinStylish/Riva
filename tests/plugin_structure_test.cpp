#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "riva/json_utils.hpp"

namespace {

void Expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAILED: " << message << "\n";
    std::exit(1);
  }
}

bool Contains(const std::string& text, const std::string& substring) {
  return text.find(substring) != std::string::npos;
}

std::string ResolvePath(const std::string& rel_path) {
  std::string p1 = rel_path;
  std::string p2 = "../" + rel_path;
  std::ifstream f1(p1);
  if (f1.is_open()) {
    return p1;
  }
  std::ifstream f2(p2);
  if (f2.is_open()) {
    return p2;
  }
  return p1;
}

std::string ReadFileContent(const std::string& rel_path) {
  const std::string path = ResolvePath(rel_path);
  std::ifstream file(path);
  Expect(file.is_open(), ("Required plugin file must exist and be readable: " + rel_path).c_str());
  std::ostringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

void TestPluginDescriptor() {
  const std::string content = ReadFileContent("ue/Plugins/RivaEditor/RivaEditor.uplugin");
  riva::JsonValue descriptor;
  riva::JsonParser parser(content);
  Expect(parser.Parse(descriptor), "Plugin descriptor must be valid JSON");
  Expect(descriptor.type == riva::JsonValue::Type::kObject,
         "Plugin descriptor root must be a JSON object");
  Expect(Contains(content, "\"Name\": \"RivaEditor\""), "Descriptor must name module RivaEditor");
  Expect(Contains(content, "\"Name\": \"RivaCore\""), "Descriptor must name module RivaCore");
  Expect(Contains(content, "\"Type\": \"Editor\""), "Descriptor module type must be Editor");
  Expect(Contains(content, "\"Type\": \"Runtime\""), "Descriptor must declare RivaCore as Runtime");
  Expect(Contains(content, "\"LoadingPhase\": \"Default\""),
         "Descriptor loading phase must be Default");
  Expect(Contains(content, "\"Category\": \"Performance\""),
         "Descriptor category must be Performance");
  Expect(Contains(content, "\"EngineVersion\": \"5.4.0\""),
         "Descriptor must target Unreal Engine 5.4");
}

void TestNoFabricatedTraceFixture() {
  const std::string path = ResolvePath("samples/sample_session.utrace");
  std::ifstream file(path, std::ios::binary);
  if (!file.is_open()) {
    return;
  }

  std::string prefix(128, '\0');
  file.read(prefix.data(), static_cast<std::streamsize>(prefix.size()));
  prefix.resize(static_cast<std::size_t>(file.gcount()));
  Expect(prefix.rfind("# Unreal Insights", 0) != 0,
         "A text description must not be shipped with a .utrace extension");
  Expect(!Contains(prefix, "[BINARY_HEADER_MAGIC_UTRACE_01]"),
         "A fabricated binary marker must not be shipped as a trace fixture");
}

void TestBuildRules() {
  const std::string content =
      ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/RivaEditor.Build.cs");
  Expect(Contains(content, "public class RivaEditor : ModuleRules"),
         "Build rules must declare RivaEditor class");
  Expect(Contains(content, "\"Slate\""), "Build rules must depend on Slate");
  Expect(Contains(content, "\"SlateCore\""), "Build rules must depend on SlateCore");
  Expect(Contains(content, "\"ToolMenus\""), "Build rules must depend on ToolMenus");
  Expect(Contains(content, "\"RivaCore\""), "Build rules must depend on RivaCore module");
  Expect(Contains(content, "\"TraceServices\""), "Build rules must depend on TraceServices");

  const std::string core_rules =
      ReadFileContent("ue/Plugins/RivaEditor/Source/RivaCore/RivaCore.Build.cs");
  Expect(Contains(core_rules, "CppStandardVersion.Cpp20"), "RivaCore must request C++20");
  Expect(Contains(core_rules, "bEnableExceptions = true"), "RivaCore must enable C++ exceptions");

  const std::string core_module =
      ReadFileContent("ue/Plugins/RivaEditor/Source/RivaCore/Private/RivaCoreModule.cpp");
  Expect(Contains(core_module, "IMPLEMENT_MODULE(FDefaultModuleImpl, RivaCore)"),
         "RivaCore must register an Unreal module implementation");

  const std::string analysis_header =
      ReadFileContent("ue/Plugins/RivaEditor/Source/RivaCore/Public/riva/analysis_engine.hpp");
  const std::string loader_header =
      ReadFileContent("ue/Plugins/RivaEditor/Source/RivaCore/Public/riva/json_trace_loader.hpp");
  const std::string report_header =
      ReadFileContent("ue/Plugins/RivaEditor/Source/RivaCore/Public/riva/report_engine.hpp");
  Expect(Contains(analysis_header, "class RIVACORE_API AnalysisEngine"),
         "AnalysisEngine must be exported across Unreal modules");
  Expect(Contains(loader_header, "RIVACORE_API JsonTraceLoadResult"),
         "JSON loader entry points must be exported across Unreal modules");
  Expect(Contains(report_header, "class RIVACORE_API FReportEngine"),
         "Report engine must be exported across Unreal modules");
}

void TestModuleImplementation() {
  const std::string header =
      ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/Public/RivaEditorModule.h");
  Expect(Contains(header, "class FRivaEditorModule : public IModuleInterface"),
         "Header must declare FRivaEditorModule inheriting IModuleInterface");
  Expect(Contains(header, "OnSpawnPluginTab"), "Header must declare tab spawner callback");

  const std::string source =
      ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/Private/RivaEditorModule.cpp");
  Expect(Contains(source, "RivaEditorTabName(\"RivaEditorTab\")"),
         "Source must define RivaEditorTab tab name");
  Expect(Contains(source, "RegisterNomadTabSpawner"), "Source must register nomad tab spawner");
  Expect(Contains(source, "LevelEditor.MainMenu.Window"),
         "Source must extend LevelEditor window menu");
  Expect(Contains(source, "SNew(SRivaPanel)"), "Tab spawner must instantiate SRivaPanel");
  Expect(Contains(source, "IMPLEMENT_MODULE(FRivaEditorModule, RivaEditor)"),
         "Source must implement Unreal module RivaEditor");
}

void TestSlateWidget() {
  const std::string header =
      ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/Public/SRivaPanel.h");
  Expect(Contains(header, "class SRivaPanel : public SCompoundWidget"),
         "Header must declare SRivaPanel inheriting SCompoundWidget");
  Expect(Contains(header, "SLATE_BEGIN_ARGS(SRivaPanel)"), "Header must use Slate arguments macro");
  Expect(Contains(header, "struct FRivaUiFinding"),
         "Header must declare FRivaUiFinding item struct");
  Expect(Contains(header, "SListView"), "Header must declare SListView member for findings");
  Expect(Contains(header, "OnGenerateFindingRow"),
         "Header must declare OnGenerateFindingRow callback");
  Expect(Contains(header, "OnFindingSelectionChanged"),
         "Header must declare OnFindingSelectionChanged callback");
  Expect(Contains(header, "OnOpenTraceClicked"), "Header must declare OnOpenTraceClicked callback");
  Expect(Contains(header, "OnAnalyzeClicked"), "Header must declare OnAnalyzeClicked callback");
  Expect(Contains(header, "OnExportMarkdownClicked"),
         "Header must declare OnExportMarkdownClicked callback");
  Expect(Contains(header, "OnExportJsonClicked"),
         "Header must declare OnExportJsonClicked callback");

  const std::string source =
      ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/Private/SRivaPanel.cpp");
  Expect(Contains(source, "DEFINE_LOG_CATEGORY_STATIC(LogRivaEditor"),
         "Source must define LogRivaEditor category");
  Expect(Contains(source, "LOCTEXT_NAMESPACE \"SRivaPanel\""),
         "Source must set localization namespace");
  Expect(Contains(source, "SSplitter"), "Source must construct SSplitter for split view");
  Expect(Contains(source, "Orient_Horizontal"), "Splitter must be oriented horizontally");
  Expect(Contains(source, "SScrollBox"), "Source must construct SScrollBox for details pane");
  Expect(Contains(source, "Open Trace..."), "Source must render Open Trace toolbar button");
  Expect(Contains(source, "Analyze"), "Source must render Analyze toolbar button");
  Expect(Contains(source, "Export Markdown..."),
         "Source must render Export Markdown toolbar button");
  Expect(Contains(source, "Export JSON..."), "Source must render Export JSON toolbar button");
  Expect(Contains(source, "Detected Hitches & Stalls"), "Source must render findings list header");
  Expect(Contains(source, "Diagnostic Evidence & Actionable Guidance"),
         "Source must render details pane header");
  Expect(Contains(source, "Status: Ready. Please open a trace file."),
         "Source must render status bar ready state");
}

void TestCoreIntegration() {
  const std::string header =
      ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/Public/RivaTraceService.h");
  Expect(Contains(header, "class FRivaTraceService"),
         "Header must declare FRivaTraceService class");
  Expect(Contains(header, "LoadAndAnalyzeJsonTrace"),
         "Header must declare LoadAndAnalyzeJsonTrace static method");

  const std::string source =
      ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/Private/RivaTraceService.cpp");
  Expect(Contains(source, "riva/json_trace_loader.hpp"),
         "Source must include riva/json_trace_loader.hpp");
  Expect(Contains(source, "riva/analysis_engine.hpp"),
         "Source must include riva/analysis_engine.hpp");
  Expect(Contains(source, "TCHAR_TO_UTF8"), "Source must convert FString to UTF8 std::string");
  Expect(Contains(source, "LoadNormalizedTraceFromJsonFile"),
         "Source must invoke the production JSON loader API");
  Expect(!Contains(source, "riva::JsonTraceLoader Loader"),
         "Source must not use the removed JsonTraceLoader class API");
  Expect(Contains(source, "Engine.Analyze"), "Source must invoke AnalysisEngine::Analyze");

  const std::string panel_source =
      ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/Private/SRivaPanel.cpp");
  Expect(Contains(panel_source, "FRivaTraceService::LoadAndAnalyzeTrace"),
         "Panel must call FRivaTraceService::LoadAndAnalyzeTrace");
  Expect(Contains(panel_source, "EAsyncExecution::ThreadPool"),
         "Panel must execute analysis on thread pool in UBT builds");
  Expect(Contains(panel_source, "AsyncTask(ENamedThreads::GameThread"),
         "Panel must marshal UI updates to the game thread");
  Expect(Contains(panel_source, "TWeakPtr<SRivaPanel>"),
         "Async analysis must not capture a raw panel pointer");
}

void TestTraceServicesLoader() {
  const std::string build_cs =
      ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/RivaEditor.Build.cs");
  Expect(Contains(build_cs, "\"TraceServices\""), "Build rules must depend on TraceServices");
  Expect(Contains(build_cs, "\"TraceLog\""), "Build rules must depend on TraceLog");
  Expect(Contains(build_cs, "\"TraceAnalysis\""), "Build rules must depend on TraceAnalysis");

  const std::string header =
      ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/Public/RivaTraceService.h");
  Expect(Contains(header, "struct FRivaNormalizedTraceSummary"),
         "Header must declare FRivaNormalizedTraceSummary");
  Expect(Contains(header, "bFrameProviderAvailable"),
         "Summary must track frame-provider availability");
  Expect(Contains(header, "bTimingProfilerAvailable"),
         "Summary must track timing-provider availability");
  Expect(Contains(header, "LoadAndAnalyzeUTrace"), "Header must declare LoadAndAnalyzeUTrace");
  Expect(Contains(header, "LoadAndAnalyzeTrace"), "Header must declare LoadAndAnalyzeTrace");

  const std::string source =
      ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/Private/RivaTraceService.cpp");
  Expect(Contains(source, "GetAnalysisService"),
         "Source must obtain the real TraceServices analysis service");
  Expect(Contains(source, "StartAnalysis(*UTraceFilePath)"),
         "Source must analyze the selected trace file");
  Expect(Contains(source, "Session->Wait()"), "Source must wait for trace analysis completion");
  Expect(Contains(source, "ReadFrameProvider"),
         "Source must read the TraceServices frame provider");
  Expect(Contains(source, "EnumerateFrames"), "Source must enumerate real frame boundaries");
  Expect(Contains(source, "GetCpuThreadTimelineIndex"), "Source must resolve CPU timing timelines");
  Expect(Contains(source, "ReadTimeline"), "Source must read real timing events");
  Expect(Contains(source, "PopulateUiFindings(Trace"),
         "Source must pass the normalized native trace to the analysis engine");
  Expect(!Contains(source, "CreateAnalysisSession(1"),
         "Source must not create an empty analysis session");
}

void TestNoFakeInsightsSync() {
  const std::string panel_cpp =
      ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/Private/SRivaPanel.cpp");
  Expect(!Contains(panel_cpp, "Sync Insights"),
         "Panel must not advertise synchronization that is not integrated");
  const std::string service_h =
      ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/Public/RivaTraceService.h");
  Expect(!Contains(service_h, "BroadcastTimeRangeSelection"),
         "Service must not expose a logging-only synchronization API");
}

void TestTraceProviderStatus() {
  const std::string header =
      ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/Public/RivaTraceService.h");
  Expect(Contains(header, "bFrameProviderAvailable"),
         "Summary must report frame-provider availability");
  Expect(Contains(header, "bTimingProfilerAvailable"),
         "Summary must report timing-provider availability");

  const std::string source =
      ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/Private/RivaTraceService.cpp");
  Expect(Contains(source, "RivaBudget.json"), "Plugin must look for a project-local Riva budget");
  Expect(!Contains(source, "../../../budgets.json"),
         "Plugin must not use a process-relative budget path");
}

void TestExportActions() {
  const std::string build_cs =
      ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/RivaEditor.Build.cs");
  Expect(Contains(build_cs, "\"DesktopPlatform\""), "Build rules must depend on DesktopPlatform");

  const std::string panel_source =
      ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/Private/SRivaPanel.cpp");
  Expect(Contains(panel_source, "FDesktopPlatformModule::Get()"),
         "Panel source must get DesktopPlatform");
  Expect(Contains(panel_source, "DesktopPlatform->SaveFileDialog"),
         "Panel source must invoke SaveFileDialog");
  Expect(Contains(panel_source, "ExportLastAnalysisToMarkdown"),
         "Panel source must call ExportLastAnalysisToMarkdown");
  Expect(Contains(panel_source, "ExportLastAnalysisToJson"),
         "Panel source must call ExportLastAnalysisToJson");

  const std::string service_header =
      ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/Public/RivaTraceService.h");
  Expect(Contains(service_header, "ExportLastAnalysisToMarkdown(const FString&"),
         "Service header must declare ExportLastAnalysisToMarkdown");
  Expect(Contains(service_header, "ExportLastAnalysisToJson(const FString&"),
         "Service header must declare ExportLastAnalysisToJson");

  const std::string service_source =
      ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/Private/RivaTraceService.cpp");
  Expect(Contains(service_source, "GLastAnalysisResult"),
         "Service source must cache GLastAnalysisResult");
  Expect(Contains(service_source, "FFileHelper::SaveStringToFile"),
         "Service source must use FFileHelper::SaveStringToFile");
}

void TestAsyncResultOrdering() {
  const std::string panel_header =
      ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/Public/SRivaPanel.h");
  const std::string panel_source =
      ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/Private/SRivaPanel.cpp");
  const std::string service_header =
      ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/Public/RivaTraceService.h");
  const std::string service_source =
      ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/Private/RivaTraceService.cpp");

  Expect(Contains(panel_header, "ActiveAnalysisRequestId"),
         "Panel must track the active background-analysis request");
  Expect(Contains(panel_source, "RequestId != ActiveAnalysisRequestId"),
         "Panel must ignore stale background-analysis completions");
  Expect(Contains(panel_source, "InvalidateAnalysisResults"),
         "Opening a trace must invalidate the prior export result");
  Expect(Contains(service_header, "BeginAnalysisRequest"),
         "Trace service must expose request generation");
  Expect(Contains(service_source, "RequestId == GLatestAnalysisRequestId"),
         "Trace service must not cache an obsolete analysis result");
}

void TestCopyActions() {
  const std::string panel_source =
      ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/Private/SRivaPanel.cpp");
  Expect(Contains(panel_source, "FPlatformApplicationMisc::ClipboardCopy"),
         "Panel source must use ClipboardCopy");
  Expect(Contains(panel_source, "OnCopySummaryClicked"),
         "Panel source must implement OnCopySummaryClicked");
  Expect(Contains(panel_source, "OnCopyTimeWindowClicked"),
         "Panel source must implement OnCopyTimeWindowClicked");

  const std::string panel_header =
      ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/Public/SRivaPanel.h");
  Expect(Contains(panel_header, "OnCopySummaryClicked"),
         "Panel header must declare OnCopySummaryClicked");
  Expect(Contains(panel_header, "OnCopyTimeWindowClicked"),
         "Panel header must declare OnCopyTimeWindowClicked");
}

}  // namespace

int main() {
  TestPluginDescriptor();
  TestNoFabricatedTraceFixture();
  TestBuildRules();
  TestModuleImplementation();
  TestSlateWidget();
  TestCoreIntegration();
  TestTraceServicesLoader();
  TestNoFakeInsightsSync();
  TestTraceProviderStatus();
  TestExportActions();
  TestAsyncResultOrdering();
  TestCopyActions();
  std::cout << "All Unreal Engine plugin structure tests passed successfully!\n";
  return 0;
}
