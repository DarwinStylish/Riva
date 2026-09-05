#include "RivaTraceService.h"

#include <memory>
#include <string>
#include <utility>

#include "HAL/FileManager.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"
#include "Modules/ModuleManager.h"
#include "TraceServices/AnalysisService.h"
#include "TraceServices/ITraceServicesModule.h"
#include "TraceServices/Model/AnalysisSession.h"
#include "TraceServices/Model/Frames.h"
#include "TraceServices/Model/Threads.h"
#include "TraceServices/Model/TimingProfiler.h"
#include "riva/analysis_engine.hpp"
#include "riva/builtin_signatures.hpp"
#include "riva/json_trace_loader.hpp"
#include "riva/report_engine.hpp"

namespace {
FCriticalSection GAnalysisResultMutex;
std::unique_ptr<riva::AnalysisResult> GLastAnalysisResult;
uint64 GLatestAnalysisRequestId = 0;

FString ToFString(const std::string& Value) { return FString(UTF8_TO_TCHAR(Value.c_str())); }

const TCHAR* EvidenceClassificationName(riva::EEvidenceClassification Classification) {
  switch (Classification) {
    case riva::EEvidenceClassification::kObserved:
      return TEXT("Observed");
    case riva::EEvidenceClassification::kDerived:
      return TEXT("Derived");
    case riva::EEvidenceClassification::kCorrelated:
      return TEXT("Correlated");
    case riva::EEvidenceClassification::kInferred:
      return TEXT("Inferred");
    case riva::EEvidenceClassification::kSuspected:
      return TEXT("Suspected");
    case riva::EEvidenceClassification::kRecommended:
      return TEXT("Recommended");
  }
  return TEXT("Unknown");
}

riva::EThreadType ClassifyThread(const FString& ThreadName) {
  if (ThreadName.Contains(TEXT("GameThread"), ESearchCase::IgnoreCase)) {
    return riva::EThreadType::kGameThread;
  }
  if (ThreadName.Contains(TEXT("RenderThread"), ESearchCase::IgnoreCase)) {
    return riva::EThreadType::kRenderThread;
  }
  if (ThreadName.Contains(TEXT("RHIThread"), ESearchCase::IgnoreCase)) {
    return riva::EThreadType::kRhiThread;
  }
  if (ThreadName.Contains(TEXT("Audio"), ESearchCase::IgnoreCase)) {
    return riva::EThreadType::kAudioThread;
  }
  if (ThreadName.Contains(TEXT("Load"), ESearchCase::IgnoreCase)) {
    return riva::EThreadType::kLoadingThread;
  }
  if (ThreadName.Contains(TEXT("Task"), ESearchCase::IgnoreCase) ||
      ThreadName.Contains(TEXT("Worker"), ESearchCase::IgnoreCase)) {
    return riva::EThreadType::kWorkerThread;
  }
  return riva::EThreadType::kCustom;
}

bool LoadAnalysisConfig(riva::AnalysisConfig& OutConfig, bool& bOutBudgetConfigured,
                        FString& OutErrorMessage) {
  bOutBudgetConfigured = false;
  const FString BudgetPath = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("RivaBudget.json"));
  if (!FPaths::FileExists(BudgetPath)) {
    return true;
  }

  const auto BudgetResult = riva::LoadBudgetConfigFromJsonFile(TCHAR_TO_UTF8(*BudgetPath));
  if (!BudgetResult.status.ok() || !BudgetResult.config.has_value()) {
    OutErrorMessage = FString::Printf(TEXT("Could not load project budget '%s': %s"), *BudgetPath,
                                      *ToFString(BudgetResult.status.message()));
    return false;
  }

  OutConfig.budget = *BudgetResult.config;
  bOutBudgetConfigured = true;
  return true;
}

bool PopulateUiFindings(const riva::NormalizedTrace& Trace, TArray<FRivaUiFinding>& OutFindings,
                        FRivaUiBudgetStatus& OutBudgetStatus, FString& OutErrorMessage,
                        uint64 RequestId) {
  riva::AnalysisConfig Config;
  if (!LoadAnalysisConfig(Config, OutBudgetStatus.bConfigured, OutErrorMessage)) {
    return false;
  }

  riva::AnalysisEngine Engine(riva::CreateBuiltinSignatures(), Config);
  riva::AnalysisResult Result = Engine.Analyze(Trace);

  {
    FScopeLock Lock(&GAnalysisResultMutex);
    if (RequestId == GLatestAnalysisRequestId) {
      GLastAnalysisResult = std::make_unique<riva::AnalysisResult>(Result);
    }
  }

  OutBudgetStatus.bBreached = Result.budget_status.breached;
  OutBudgetStatus.BreachedMetrics.Empty();
  for (const std::string& Metric : Result.budget_status.breached_metrics) {
    OutBudgetStatus.BreachedMetrics.Add(ToFString(Metric));
  }

  OutFindings.Empty();
  for (const riva::ResolvedFinding& Resolved : Result.findings) {
    const riva::Finding& Finding = Resolved.finding;
    const bool bIsPrimary = Resolved.role == riva::FindingRole::kPrimary;
    const double StartTimeMs = static_cast<double>(Finding.time_window_start_us) / 1000.0;
    const double EndTimeMs = static_cast<double>(Finding.time_window_end_us) / 1000.0;

    FRivaUiFinding UiFinding;
    UiFinding.Title = FText::FromString(ToFString(Finding.title));
    UiFinding.Role = bIsPrimary ? NSLOCTEXT("RivaTraceService", "RolePrimary", "[Primary]")
                                : NSLOCTEXT("RivaTraceService", "RoleSecondary", "[Secondary]");
    UiFinding.Confidence =
        FText::Format(NSLOCTEXT("RivaTraceService", "ConfidenceFmt", "Confidence: {0}%"),
                      FText::AsNumber(FMath::RoundToInt(Finding.confidence * 100.0)));
    UiFinding.TimeWindow =
        FText::Format(NSLOCTEXT("RivaTraceService", "TimeWindowFmt", "{0} ms - {1} ms"),
                      FText::AsNumber(StartTimeMs), FText::AsNumber(EndTimeMs));
    UiFinding.StartTimeMs = StartTimeMs;
    UiFinding.EndTimeMs = EndTimeMs;

    FString Details = FString::Printf(
        TEXT("Role: %s\nAffected thread: %s\nAffected system: %s\n\nEvidence Breakdown:\n"),
        bIsPrimary ? TEXT("Primary") : TEXT("Secondary"),
        Finding.affected_thread.empty() ? TEXT("Not identified")
                                        : *ToFString(Finding.affected_thread),
        Finding.affected_system.empty() ? TEXT("Not identified")
                                        : *ToFString(Finding.affected_system));

    if (Finding.evidence.empty()) {
      Details += TEXT("- No supporting evidence was emitted.\n");
    } else {
      for (const riva::Evidence& Evidence : Finding.evidence) {
        Details += FString::Printf(TEXT("- [%s] %s: %s\n"),
                                   EvidenceClassificationName(Evidence.classification),
                                   *ToFString(Evidence.label), *ToFString(Evidence.value));
      }
    }

    if (!Finding.suggested_next_steps.empty()) {
      Details += TEXT("\nSuggested Next Steps:\n");
      for (size_t Index = 0; Index < Finding.suggested_next_steps.size(); ++Index) {
        Details += FString::Printf(TEXT("%d. %s\n"), static_cast<int32>(Index + 1),
                                   *ToFString(Finding.suggested_next_steps[Index]));
      }
    }

    if (!Finding.how_to_confirm.empty()) {
      Details += TEXT("\nHow to Confirm in Unreal Insights:\n");
      for (const std::string& Confirmation : Finding.how_to_confirm) {
        Details += FString::Printf(TEXT("- %s\n"), *ToFString(Confirmation));
      }
    }

    UiFinding.DetailedReport = FText::FromString(Details);
    OutFindings.Add(std::move(UiFinding));
  }

  return true;
}

bool ExtractUTrace(const FString& UTraceFilePath, riva::NormalizedTrace& OutTrace,
                   FRivaNormalizedTraceSummary& OutSummary, FString& OutErrorMessage) {
  OutSummary = FRivaNormalizedTraceSummary{};
  OutSummary.SourceFormat = TEXT("utrace");
  OutSummary.TraceFilePath = UTraceFilePath;

  if (UTraceFilePath.IsEmpty()) {
    OutErrorMessage = TEXT("Trace file path cannot be empty.");
    return false;
  }
  if (!FPaths::FileExists(UTraceFilePath)) {
    OutErrorMessage = FString::Printf(TEXT("Trace file does not exist: %s"), *UTraceFilePath);
    return false;
  }
  if (IFileManager::Get().FileSize(*UTraceFilePath) <= 0) {
    OutErrorMessage = FString::Printf(TEXT("Trace file is empty: %s"), *UTraceFilePath);
    return false;
  }

  ITraceServicesModule& TraceServicesModule =
      FModuleManager::GetModuleChecked<ITraceServicesModule>(TEXT("TraceServices"));
  const TSharedPtr<TraceServices::IAnalysisService> AnalysisService =
      TraceServicesModule.GetAnalysisService();
  if (!AnalysisService.IsValid()) {
    OutErrorMessage = TEXT("TraceServices did not provide an analysis service.");
    return false;
  }

  const TSharedPtr<const TraceServices::IAnalysisSession> Session =
      AnalysisService->StartAnalysis(*UTraceFilePath);
  if (!Session.IsValid()) {
    OutErrorMessage =
        FString::Printf(TEXT("Unreal TraceServices could not open trace: %s"), *UTraceFilePath);
    return false;
  }

  Session->Wait();
  if (!Session->IsAnalysisComplete()) {
    OutErrorMessage = TEXT("TraceServices analysis did not complete.");
    return false;
  }

  TraceServices::FAnalysisSessionReadScope SessionReadScope(*Session);
  const TraceServices::IFrameProvider& FrameProvider = TraceServices::ReadFrameProvider(*Session);
  const TraceServices::IThreadProvider& ThreadProvider =
      TraceServices::ReadThreadProvider(*Session);
  const TraceServices::ITimingProfilerProvider& TimingProvider =
      TraceServices::ReadTimingProfilerProvider(*Session);

  uint32 GameThreadId = 0;
  uint32 RenderThreadId = 0;
  uint32 RhiThreadId = 0;
  ThreadProvider.EnumerateThreads([&](const TraceServices::FThreadInfo& ThreadInfo) {
    const FString ThreadName = ThreadInfo.Name != nullptr ? ThreadInfo.Name : TEXT("");
    if (ThreadName.Contains(TEXT("GameThread"), ESearchCase::IgnoreCase)) {
      GameThreadId = ThreadInfo.Id;
    } else if (ThreadName.Contains(TEXT("RenderThread"), ESearchCase::IgnoreCase)) {
      RenderThreadId = ThreadInfo.Id;
    } else if (ThreadName.Contains(TEXT("RHIThread"), ESearchCase::IgnoreCase)) {
      RhiThreadId = ThreadInfo.Id;
    }

    riva::FTraceThread RivaThread;
    RivaThread.id = ThreadInfo.Id;
    RivaThread.name = TCHAR_TO_UTF8(*ThreadName);
    RivaThread.type = ClassifyThread(ThreadName);
    static_cast<void>(OutTrace.AddThread(std::move(RivaThread)));
  });

  auto ReadTimelineDurationMs = [&](uint32 TimelineIndex,
                                    const TraceServices::FFrame& Frame) -> double {
    double DurationMs = 0.0;
    TimingProvider.ReadTimeline(
        TimelineIndex, [&](const TraceServices::ITimingProfilerProvider::Timeline& Timeline) {
          Timeline.EnumerateEvents(
              Frame.StartTime, Frame.EndTime,
              [&](double EventStart, double EventEnd, uint32 Depth,
                  const TraceServices::FTimingProfilerEvent& Event) {
                static_cast<void>(Event);
                if (Depth == 0) {
                  const double ClampedStart = FMath::Max(EventStart, Frame.StartTime);
                  const double ClampedEnd = FMath::Min(EventEnd, Frame.EndTime);
                  if (ClampedEnd > ClampedStart) {
                    DurationMs += (ClampedEnd - ClampedStart) * 1000.0;
                  }
                }
                return TraceServices::EEventEnumerate::Continue;
              });
        });
    return DurationMs;
  };

  auto CpuTimelineIndex = [&](uint32 ThreadId, uint32& OutTimelineIndex) -> bool {
    return ThreadId != 0 && TimingProvider.GetCpuThreadTimelineIndex(ThreadId, OutTimelineIndex);
  };

  uint32 GameTimelineIndex = 0;
  uint32 RenderTimelineIndex = 0;
  uint32 RhiTimelineIndex = 0;
  uint32 GpuTimelineIndex = 0;
  const bool bHasGameTimeline = CpuTimelineIndex(GameThreadId, GameTimelineIndex);
  const bool bHasRenderTimeline = CpuTimelineIndex(RenderThreadId, RenderTimelineIndex);
  const bool bHasRhiTimeline = CpuTimelineIndex(RhiThreadId, RhiTimelineIndex);
  const bool bHasGpuTimeline = TimingProvider.GetGpuTimelineIndex(GpuTimelineIndex);
  OutSummary.bTimingProfilerAvailable =
      bHasGameTimeline || bHasRenderTimeline || bHasRhiTimeline || bHasGpuTimeline;

  const uint64 FrameCount = FrameProvider.GetFrameCount(TraceServices::TraceFrameType_Game);
  if (FrameCount == 0) {
    OutErrorMessage = TEXT(
        "The trace contains no game-frame boundaries. Capture with the 'frame' trace channel "
        "enabled.");
    return false;
  }

  bool bFramesValid = true;
  std::string FrameError;
  std::size_t FrameIndex = 0;
  FrameProvider.EnumerateFrames(
      TraceServices::TraceFrameType_Game, static_cast<uint64>(0), FrameCount,
      [&](const TraceServices::FFrame& Frame) {
        if (!bFramesValid) {
          return;
        }

        riva::Frame RivaFrame;
        RivaFrame.index = FrameIndex++;
        RivaFrame.start_time_us =
            static_cast<std::uint64_t>(FMath::Max(0.0, Frame.StartTime) * 1000000.0);
        RivaFrame.duration_ms = FMath::Max(0.0, Frame.EndTime - Frame.StartTime) * 1000.0;
        RivaFrame.game_thread_ms =
            bHasGameTimeline ? ReadTimelineDurationMs(GameTimelineIndex, Frame) : 0.0;
        RivaFrame.render_thread_ms =
            bHasRenderTimeline ? ReadTimelineDurationMs(RenderTimelineIndex, Frame) : 0.0;
        RivaFrame.rhi_thread_ms =
            bHasRhiTimeline ? ReadTimelineDurationMs(RhiTimelineIndex, Frame) : 0.0;
        RivaFrame.gpu_ms = bHasGpuTimeline ? ReadTimelineDurationMs(GpuTimelineIndex, Frame) : 0.0;

        const riva::Status AddStatus = OutTrace.AddFrame(std::move(RivaFrame));
        if (!AddStatus.ok()) {
          bFramesValid = false;
          FrameError = AddStatus.message();
          return;
        }

        ++OutSummary.TotalFrames;
        OutSummary.TotalDurationMs += FMath::Max(0.0, Frame.EndTime - Frame.StartTime) * 1000.0;
      });

  if (!bFramesValid) {
    OutErrorMessage =
        FString::Printf(TEXT("Trace frame normalization failed: %s"), *ToFString(FrameError));
    return false;
  }

  OutSummary.bFrameProviderAvailable = OutSummary.TotalFrames > 0;
  return true;
}
}  // namespace

uint64 FRivaTraceService::BeginAnalysisRequest() {
  FScopeLock Lock(&GAnalysisResultMutex);
  ++GLatestAnalysisRequestId;
  if (GLatestAnalysisRequestId == 0) {
    ++GLatestAnalysisRequestId;
  }
  GLastAnalysisResult.reset();
  return GLatestAnalysisRequestId;
}

void FRivaTraceService::InvalidateAnalysisResults() { static_cast<void>(BeginAnalysisRequest()); }

bool FRivaTraceService::LoadAndAnalyzeTrace(uint64 RequestId, const FString& FilePath,
                                            TArray<FRivaUiFinding>& OutFindings,
                                            FRivaUiBudgetStatus& OutBudgetStatus,
                                            FString& OutErrorMessage) {
  OutFindings.Empty();
  OutBudgetStatus = FRivaUiBudgetStatus{};
  OutErrorMessage.Empty();

  const FString Extension = FPaths::GetExtension(FilePath, true).ToLower();
  if (Extension == TEXT(".utrace") ||
      FilePath.EndsWith(TEXT(".utrace.temp"), ESearchCase::IgnoreCase)) {
    return LoadAndAnalyzeUTrace(RequestId, FilePath, OutFindings, OutBudgetStatus, OutErrorMessage);
  }
  if (Extension == TEXT(".json")) {
    return LoadAndAnalyzeJsonTrace(RequestId, FilePath, OutFindings, OutBudgetStatus,
                                   OutErrorMessage);
  }

  OutErrorMessage = FString::Printf(
      TEXT("Unsupported trace format for '%s'. Select a .utrace or .json file."), *FilePath);
  return false;
}

bool FRivaTraceService::ExtractNormalizedTraceFromUTrace(const FString& UTraceFilePath,
                                                         FRivaNormalizedTraceSummary& OutSummary,
                                                         FString& OutErrorMessage) {
  riva::NormalizedTrace Trace(TCHAR_TO_UTF8(*UTraceFilePath));
  return ExtractUTrace(UTraceFilePath, Trace, OutSummary, OutErrorMessage);
}

bool FRivaTraceService::LoadAndAnalyzeUTrace(uint64 RequestId, const FString& UTraceFilePath,
                                             TArray<FRivaUiFinding>& OutFindings,
                                             FRivaUiBudgetStatus& OutBudgetStatus,
                                             FString& OutErrorMessage) {
  riva::NormalizedTrace Trace(TCHAR_TO_UTF8(*UTraceFilePath));
  FRivaNormalizedTraceSummary Summary;
  if (!ExtractUTrace(UTraceFilePath, Trace, Summary, OutErrorMessage)) {
    return false;
  }
  return PopulateUiFindings(Trace, OutFindings, OutBudgetStatus, OutErrorMessage, RequestId);
}

bool FRivaTraceService::LoadAndAnalyzeJsonTrace(uint64 RequestId, const FString& JsonFilePath,
                                                TArray<FRivaUiFinding>& OutFindings,
                                                FRivaUiBudgetStatus& OutBudgetStatus,
                                                FString& OutErrorMessage) {
  const riva::JsonTraceLoadResult LoadResult =
      riva::LoadNormalizedTraceFromJsonFile(TCHAR_TO_UTF8(*JsonFilePath));
  if (!LoadResult.status.ok() || !LoadResult.trace.has_value()) {
    OutErrorMessage = ToFString(LoadResult.status.message());
    if (OutErrorMessage.IsEmpty()) {
      OutErrorMessage = TEXT("The JSON trace loader returned no trace data.");
    }
    return false;
  }

  return PopulateUiFindings(*LoadResult.trace, OutFindings, OutBudgetStatus, OutErrorMessage,
                            RequestId);
}

bool FRivaTraceService::ExportLastAnalysisToMarkdown(const FString& OutFilePath,
                                                     FString& OutErrorMessage) {
  FScopeLock Lock(&GAnalysisResultMutex);
  if (!GLastAnalysisResult) {
    OutErrorMessage = TEXT("No analysis result is available to export.");
    return false;
  }

  std::string OutReport;
  const riva::Status Status = riva::FReportEngine::GenerateMarkdownReport(
      *GLastAnalysisResult, riva::FReportOptions{}, OutReport);
  if (!Status.ok()) {
    OutErrorMessage = ToFString(Status.message());
    return false;
  }
  if (!FFileHelper::SaveStringToFile(FString(UTF8_TO_TCHAR(OutReport.c_str())), *OutFilePath)) {
    OutErrorMessage = FString::Printf(TEXT("Could not write report: %s"), *OutFilePath);
    return false;
  }
  return true;
}

bool FRivaTraceService::ExportLastAnalysisToJson(const FString& OutFilePath,
                                                 FString& OutErrorMessage) {
  FScopeLock Lock(&GAnalysisResultMutex);
  if (!GLastAnalysisResult) {
    OutErrorMessage = TEXT("No analysis result is available to export.");
    return false;
  }

  std::string OutReport;
  const riva::Status Status = riva::FReportEngine::GenerateJsonReport(
      *GLastAnalysisResult, riva::FReportOptions{}, OutReport);
  if (!Status.ok()) {
    OutErrorMessage = ToFString(Status.message());
    return false;
  }
  if (!FFileHelper::SaveStringToFile(FString(UTF8_TO_TCHAR(OutReport.c_str())), *OutFilePath)) {
    OutErrorMessage = FString::Printf(TEXT("Could not write report: %s"), *OutFilePath);
    return false;
  }
  return true;
}
