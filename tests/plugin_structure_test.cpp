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

}  // namespace

int main() {
  TestPluginDescriptor();
  TestBuildRules();
  TestModuleImplementation();
  TestSlateWidget();
  std::cout << "All Unreal Engine plugin structure tests passed successfully!\n";
  return 0;
}
