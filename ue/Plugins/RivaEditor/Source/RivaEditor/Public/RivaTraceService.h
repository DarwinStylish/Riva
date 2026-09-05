#pragma once

#include "CoreMinimal.h"
#include "SRivaPanel.h"

struct FRivaNormalizedTraceSummary {
  FString SourceFormat;
  FString TraceFilePath;
  int32 TotalFrames;
  double TotalDurationMs;
  bool bFrameProviderAvailable;
  bool bTimingProfilerAvailable;
};

class FRivaTraceService {
 public:
  static uint64 BeginAnalysisRequest();
  static void InvalidateAnalysisResults();

  static bool LoadAndAnalyzeTrace(uint64 RequestId, const FString& FilePath,
                                  TArray<FRivaUiFinding>& OutFindings,
                                  FRivaUiBudgetStatus& OutBudgetStatus, FString& OutErrorMessage);
  static bool LoadAndAnalyzeJsonTrace(uint64 RequestId, const FString& JsonFilePath,
                                      TArray<FRivaUiFinding>& OutFindings,
                                      FRivaUiBudgetStatus& OutBudgetStatus,
                                      FString& OutErrorMessage);
  static bool LoadAndAnalyzeUTrace(uint64 RequestId, const FString& UTraceFilePath,
                                   TArray<FRivaUiFinding>& OutFindings,
                                   FRivaUiBudgetStatus& OutBudgetStatus, FString& OutErrorMessage);
  static bool ExtractNormalizedTraceFromUTrace(const FString& UTraceFilePath,
                                               FRivaNormalizedTraceSummary& OutSummary,
                                               FString& OutErrorMessage);

  static bool ExportLastAnalysisToMarkdown(const FString& OutFilePath, FString& OutErrorMessage);
  static bool ExportLastAnalysisToJson(const FString& OutFilePath, FString& OutErrorMessage);
};
