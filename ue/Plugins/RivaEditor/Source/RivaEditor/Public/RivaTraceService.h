#pragma once

#include "CoreMinimal.h"
#include "SRivaPanel.h"

struct FRivaNormalizedTraceSummary
{
    FString SourceFormat;
    FString TraceFilePath;
    int32 TotalFrames;
    double TotalDurationMs;
    int32 TotalMarkers;
    bool bMarkerProviderAvailable;
};

class FRivaTraceService
{
public:
    static bool LoadAndAnalyzeTrace(const FString& FilePath, TArray<FRivaUiFinding>& OutFindings, FString& OutErrorMessage);
    static bool LoadAndAnalyzeJsonTrace(const FString& JsonFilePath, TArray<FRivaUiFinding>& OutFindings, FString& OutErrorMessage);
    static bool LoadAndAnalyzeUTrace(const FString& UTraceFilePath, TArray<FRivaUiFinding>& OutFindings, FString& OutErrorMessage);
    static bool ExtractNormalizedTraceFromUTrace(const FString& UTraceFilePath, FRivaNormalizedTraceSummary& OutSummary, FString& OutErrorMessage);

    static void BroadcastTimeRangeSelection(double StartTimeMs, double EndTimeMs);
    static void RegisterInsightsSelectionCallback(TFunction<void(double, double)> Callback);
    static void UnregisterInsightsSelectionCallback();
    static void SimulateInsightsSelection(double StartTimeMs, double EndTimeMs);
    static bool ExportLastAnalysisToMarkdown(const FString& OutFilePath, FString& OutErrorMessage);
    static bool ExportLastAnalysisToJson(const FString& OutFilePath, FString& OutErrorMessage);
};
