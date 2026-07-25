#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

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
  Expect(Contains(content, "\"Name\": \"RivaEditor\""), "Descriptor must name module RivaEditor");
  Expect(Contains(content, "\"Type\": \"Editor\""), "Descriptor module type must be Editor");
  Expect(Contains(content, "\"LoadingPhase\": \"Default\""), "Descriptor loading phase must be Default");
  Expect(Contains(content, "\"Category\": \"Performance\""), "Descriptor category must be Performance");
}

void TestBuildRules() {
  const std::string content = ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/RivaEditor.Build.cs");
  Expect(Contains(content, "public class RivaEditor : ModuleRules"), "Build rules must declare RivaEditor class");
  Expect(Contains(content, "\"Slate\""), "Build rules must depend on Slate");
  Expect(Contains(content, "\"SlateCore\""), "Build rules must depend on SlateCore");
  Expect(Contains(content, "\"ToolMenus\""), "Build rules must depend on ToolMenus");
  Expect(Contains(content, "../../../include"), "Build rules must include RivaCore header directory");
}

void TestModuleImplementation() {
  const std::string header = ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/Public/RivaEditorModule.h");
  Expect(Contains(header, "class FRivaEditorModule : public IModuleInterface"),
         "Header must declare FRivaEditorModule inheriting IModuleInterface");
  Expect(Contains(header, "OnSpawnPluginTab"), "Header must declare tab spawner callback");

  const std::string source = ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/Private/RivaEditorModule.cpp");
  Expect(Contains(source, "RivaEditorTabName(\"RivaEditorTab\")"), "Source must define RivaEditorTab tab name");
  Expect(Contains(source, "RegisterNomadTabSpawner"), "Source must register nomad tab spawner");
  Expect(Contains(source, "LevelEditor.MainMenu.Window"), "Source must extend LevelEditor window menu");
  Expect(Contains(source, "SNew(SRivaPanel)"), "Tab spawner must instantiate SRivaPanel");
  Expect(Contains(source, "IMPLEMENT_MODULE(FRivaEditorModule, RivaEditor)"),
         "Source must implement Unreal module RivaEditor");
}

void TestSlateWidget() {
  const std::string header = ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/Public/SRivaPanel.h");
  Expect(Contains(header, "class SRivaPanel : public SCompoundWidget"),
         "Header must declare SRivaPanel inheriting SCompoundWidget");
  Expect(Contains(header, "SLATE_BEGIN_ARGS(SRivaPanel)"), "Header must use Slate arguments macro");
  Expect(Contains(header, "struct FRivaUiFinding"), "Header must declare FRivaUiFinding item struct");
  Expect(Contains(header, "SListView"), "Header must declare SListView member for findings");
  Expect(Contains(header, "OnGenerateFindingRow"), "Header must declare OnGenerateFindingRow callback");
  Expect(Contains(header, "OnFindingSelectionChanged"), "Header must declare OnFindingSelectionChanged callback");
  Expect(Contains(header, "OnOpenTraceClicked"), "Header must declare OnOpenTraceClicked callback");
  Expect(Contains(header, "OnAnalyzeClicked"), "Header must declare OnAnalyzeClicked callback");
  Expect(Contains(header, "OnExportMarkdownClicked"), "Header must declare OnExportMarkdownClicked callback");
  Expect(Contains(header, "OnExportJsonClicked"), "Header must declare OnExportJsonClicked callback");

  const std::string source = ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/Private/SRivaPanel.cpp");
  Expect(Contains(source, "DEFINE_LOG_CATEGORY_STATIC(LogRivaEditor"), "Source must define LogRivaEditor category");
  Expect(Contains(source, "LOCTEXT_NAMESPACE \"SRivaPanel\""), "Source must set localization namespace");
  Expect(Contains(source, "SSplitter"), "Source must construct SSplitter for split view");
  Expect(Contains(source, "Orient_Horizontal"), "Splitter must be oriented horizontally");
  Expect(Contains(source, "SScrollBox"), "Source must construct SScrollBox for details pane");
  Expect(Contains(source, "Open Trace..."), "Source must render Open Trace toolbar button");
  Expect(Contains(source, "Analyze"), "Source must render Analyze toolbar button");
  Expect(Contains(source, "Export Markdown..."), "Source must render Export Markdown toolbar button");
  Expect(Contains(source, "Export JSON..."), "Source must render Export JSON toolbar button");
  Expect(Contains(source, "Detected Hitches & Stalls"), "Source must render findings list header");
  Expect(Contains(source, "Diagnostic Evidence & Actionable Guidance"), "Source must render details pane header");
  Expect(Contains(source, "Status: Ready for trace analysis"), "Source must render status bar ready state");
}

void TestCoreIntegration() {
  const std::string header = ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/Public/RivaTraceService.h");
  Expect(Contains(header, "class FRivaTraceService"), "Header must declare FRivaTraceService class");
  Expect(Contains(header, "LoadAndAnalyzeJsonTrace"), "Header must declare LoadAndAnalyzeJsonTrace static method");

  const std::string source = ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/Private/RivaTraceService.cpp");
  Expect(Contains(source, "riva/json_loader.h"), "Source must include riva/json_loader.h");
  Expect(Contains(source, "riva/analysis_engine.h"), "Source must include riva/analysis_engine.h");
  Expect(Contains(source, "TCHAR_TO_UTF8"), "Source must convert FString to UTF8 std::string");
  Expect(Contains(source, "Loader.LoadTrace(StdFilePath)"), "Source must invoke JsonTraceLoader::LoadTrace");
  Expect(Contains(source, "Engine.Analyze"), "Source must invoke AnalysisEngine::Analyze");

  const std::string panel_source = ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/Private/SRivaPanel.cpp");
  Expect(Contains(panel_source, "FRivaTraceService::LoadAndAnalyzeTrace"), "Panel must call FRivaTraceService::LoadAndAnalyzeTrace");
  Expect(Contains(panel_source, "EAsyncExecution::ThreadPool"), "Panel must execute analysis on thread pool in UBT builds");
  Expect(Contains(panel_source, "EAsyncExecution::TaskGraphMainThread"), "Panel must marshal UI updates to main thread in UBT builds");
}

void TestTraceServicesLoader() {
  const std::string build_cs = ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/RivaEditor.Build.cs");
  Expect(Contains(build_cs, "\"TraceServices\""), "Build rules must depend on TraceServices");
  Expect(Contains(build_cs, "\"TraceLog\""), "Build rules must depend on TraceLog");
  Expect(Contains(build_cs, "\"TraceAnalysis\""), "Build rules must depend on TraceAnalysis");

  const std::string header = ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/Public/RivaTraceService.h");
  Expect(Contains(header, "struct FRivaNormalizedTraceSummary"), "Header must declare FRivaNormalizedTraceSummary");
  Expect(Contains(header, "bMarkerProviderAvailable"), "Summary must track bMarkerProviderAvailable");
  Expect(Contains(header, "LoadAndAnalyzeUTrace"), "Header must declare LoadAndAnalyzeUTrace");
  Expect(Contains(header, "LoadAndAnalyzeTrace"), "Header must declare LoadAndAnalyzeTrace");

  const std::string source = ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/Private/RivaTraceService.cpp");
  Expect(Contains(source, "degrading gracefully"), "Source must handle marker degradation gracefully");
  Expect(Contains(source, "utrace"), "Source must format trace metadata as utrace");
  Expect(Contains(source, "Engine.Analyze(Trace)"), "Source must pass converted NormalizedTrace to AnalysisEngine");

  const std::string panel_source = ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/Private/SRivaPanel.cpp");
  Expect(Contains(panel_source, "sample_session.utrace"), "Panel must include sample_session.utrace in sample rotation");
  Expect(Contains(panel_source, "spike_shader_compile.json"), "Panel must reference exact spike_shader_compile sample filename");
}

void TestSelectionSync() {
  const std::string panel_h = ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/Public/SRivaPanel.h");
  Expect(Contains(panel_h, "double StartTimeMs"), "Panel header must declare StartTimeMs");
  Expect(Contains(panel_h, "double EndTimeMs"), "Panel header must declare EndTimeMs");
  Expect(Contains(panel_h, "bool bSyncWithInsightsEnabled"), "Panel header must declare bSyncWithInsightsEnabled");
  Expect(Contains(panel_h, "OnSyncInsightsToggled"), "Panel header must declare OnSyncInsightsToggled");

  const std::string panel_cpp = ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/Private/SRivaPanel.cpp");
  Expect(Contains(panel_cpp, "BroadcastTimeRangeSelection"), "Panel source must broadcast time range selections");
  Expect(Contains(panel_cpp, "RegisterInsightsSelectionCallback"), "Panel source must register callback");
  Expect(Contains(panel_cpp, "SimulateInsightsSelection"), "Panel source must support simulation trigger");

  const std::string service_h = ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/Public/RivaTraceService.h");
  Expect(Contains(service_h, "BroadcastTimeRangeSelection"), "Service header must declare BroadcastTimeRangeSelection");
  Expect(Contains(service_h, "RegisterInsightsSelectionCallback"), "Service header must declare callback registration");
  Expect(Contains(service_h, "SimulateInsightsSelection"), "Service header must declare SimulateInsightsSelection");

  const std::string service_cpp = ReadFileContent("ue/Plugins/RivaEditor/Source/RivaEditor/Private/RivaTraceService.cpp");
  Expect(Contains(service_cpp, "GOnInsightsRangeSelectedCallback"), "Service source must manage callback instance");
}

}  // namespace

int main() {
  TestPluginDescriptor();
  TestBuildRules();
  TestModuleImplementation();
  TestSlateWidget();
  TestCoreIntegration();
  TestTraceServicesLoader();
  TestSelectionSync();
  std::cout << "All Unreal Engine plugin structure tests passed successfully!\n";
  return 0;
}
