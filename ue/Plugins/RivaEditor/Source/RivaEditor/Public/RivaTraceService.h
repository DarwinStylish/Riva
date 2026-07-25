#pragma once

#include "CoreMinimal.h"
#include "SRivaPanel.h"

class FRivaTraceService
{
public:
    static bool LoadAndAnalyzeJsonTrace(const FString& JsonFilePath, TArray<FRivaUiFinding>& OutFindings, FString& OutErrorMessage);
};
