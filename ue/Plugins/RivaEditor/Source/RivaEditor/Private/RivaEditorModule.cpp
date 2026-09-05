#include "RivaEditorModule.h"

#include "Framework/Docking/TabManager.h"
#include "Modules/ModuleManager.h"
#include "SRivaPanel.h"
#include "ToolMenus.h"
#include "TraceServices/ITraceServicesModule.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

#define LOCTEXT_NAMESPACE "FRivaEditorModule"

static const FName RivaEditorTabName("RivaEditorTab");

void FRivaEditorModule::StartupModule() {
  // Load TraceServices on the editor thread before analyses are dispatched to
  // the background thread pool.
  FModuleManager::LoadModuleChecked<ITraceServicesModule>("TraceServices");

  FGlobalTabmanager::Get()
      ->RegisterNomadTabSpawner(RivaEditorTabName,
                                FOnSpawnTab::CreateRaw(this, &FRivaEditorModule::OnSpawnPluginTab))
      .SetDisplayName(LOCTEXT("FRivaEditorTabTitle", "Riva"))
      .SetTooltipText(LOCTEXT("FRivaEditorTabTooltip",
                              "Open the Riva performance diagnostics dockable companion tab."))
      .SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsMiscCategory())
      .SetIcon(FSlateIcon());

  UToolMenus::RegisterStartupCallback(
      FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FRivaEditorModule::RegisterMenus));
}

void FRivaEditorModule::ShutdownModule() {
  UToolMenus::UnRegisterStartupCallback(this);
  UToolMenus::UnregisterOwner(this);

  FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(RivaEditorTabName);
}

void FRivaEditorModule::RegisterMenus() {
  FToolMenuOwnerScoped OwnerScoped(this);

  UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
  if (Menu != nullptr) {
    FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
    Section.AddMenuEntryWithCommandList(
        RivaEditorTabName, LOCTEXT("RivaMenuLabel", "Riva Performance Companion"),
        LOCTEXT("RivaMenuTooltip",
                "Open the Riva deterministic performance diagnostics dockable tab."),
        FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([]() {
          FGlobalTabmanager::Get()->TryInvokeTab(RivaEditorTabName);
        })));
  }
}

TSharedRef<SDockTab> FRivaEditorModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs) {
  return SNew(SDockTab).TabRole(ETabRole::NomadTab)[SNew(SRivaPanel)];
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRivaEditorModule, RivaEditor)
