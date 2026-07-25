#include "RivaEditorModule.h"
#include "SRivaPanel.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "FRivaEditorModule"

static const FName RivaEditorTabName("RivaEditorTab");

void FRivaEditorModule::StartupModule()
{
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(RivaEditorTabName,
        FOnSpawnTab::CreateRaw(this, &FRivaEditorModule::OnSpawnPluginTab))
        .SetDisplayName(LOCTEXT("FRivaEditorTabTitle", "Riva"))
        .SetTooltipText(LOCTEXT("FRivaEditorTabTooltip", "Open the Riva performance diagnostics dockable companion tab."))
        .SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsMiscCategory())
        .SetIcon(FSlateIcon());

    RegisterMenus();
}

void FRivaEditorModule::ShutdownModule()
{
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);

    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(RivaEditorTabName);
}

void FRivaEditorModule::RegisterMenus()
{
    FToolMenuOwnerScoped OwnerScoped(this);

    UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
    if (Menu != nullptr)
    {
        FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
        Section.AddMenuEntryWithCommandList(
            RivaEditorTabName,
            LOCTEXT("RivaMenuLabel", "Riva Performance Companion"),
            LOCTEXT("RivaMenuTooltip", "Open the Riva deterministic performance diagnostics dockable tab."),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateLambda([]() {
                FGlobalTabmanager::Get()->TryInvokeTab(RivaEditorTabName);
            }))
        );
    }
}

TSharedRef<SDockTab> FRivaEditorModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(SRivaPanel)
        ];
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRivaEditorModule, RivaEditor)
