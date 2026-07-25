#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FRivaEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    void RegisterMenus();
    TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

    TSharedPtr<class FUICommandList> PluginCommands;
};
