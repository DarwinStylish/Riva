#include "RivaTraceService.h"
#include "riva/json_loader.h"
#include "riva/analysis_engine.h"
#include "riva/report_engine.h"
#include "Math/UnrealMathUtility.h"

bool FRivaTraceService::LoadAndAnalyzeJsonTrace(const FString& JsonFilePath, TArray<FRivaUiFinding>& OutFindings, FString& OutErrorMessage)
{
    const std::string StdFilePath = TCHAR_TO_UTF8(*JsonFilePath);

    riva::JsonTraceLoader Loader;
    riva::Result<riva::NormalizedTrace> LoadResult = Loader.LoadTrace(StdFilePath);
    if (!LoadResult.IsSuccess())
    {
        OutErrorMessage = FString(LoadResult.GetError().c_str());
        return false;
    }

    riva::AnalysisEngine Engine;
    riva::AnalysisReport Report = Engine.Analyze(LoadResult.GetValue());

    OutFindings.Empty();
    for (const riva::Finding& Finding : Report.findings)
    {
        FRivaUiFinding UiFinding;
        UiFinding.Title = FText::FromString(FString(Finding.title.c_str()));
        UiFinding.Role = Finding.is_primary ? NSLOCTEXT("RivaTraceService", "RolePrimary", "[Primary]") : NSLOCTEXT("RivaTraceService", "RoleSecondary", "[Secondary]");
        UiFinding.Confidence = FText::Format(NSLOCTEXT("RivaTraceService", "ConfidenceFmt", "Confidence: {0}%"), FText::AsNumber(FMath::RoundToInt(Finding.confidence * 100.0f)));
        UiFinding.TimeWindow = FText::Format(NSLOCTEXT("RivaTraceService", "TimeWindowFmt", "{0} ms - {1} ms"), FText::AsNumber(Finding.start_time_ms), FText::AsNumber(Finding.end_time_ms));

        FString DetailedStr = FString::Printf(TEXT("Executive Summary:\n%s\n\nEvidence Breakdown:\n"), *FString(Finding.summary.c_str()));
        for (const std::string& Ev : Finding.evidence)
        {
            DetailedStr += FString::Printf(TEXT("- %s\n"), *FString(Ev.c_str()));
        }
        if (!Finding.next_steps.empty())
        {
            DetailedStr += TEXT("\nActionable Guidance:\n");
            for (size_t i = 0; i < Finding.next_steps.size(); ++i)
            {
                DetailedStr += FString::Printf(TEXT("%d. %s\n"), static_cast<int>(i + 1), *FString(Finding.next_steps[i].c_str()));
            }
        }
        if (!Finding.how_to_confirm.empty())
        {
            DetailedStr += FString::Printf(TEXT("\nHow to Confirm in Unreal Insights:\n%s"), *FString(Finding.how_to_confirm.c_str()));
        }
        UiFinding.DetailedReport = FText::FromString(DetailedStr);
        OutFindings.Add(UiFinding);
    }

    return true;
}

#if !defined(RIVA_CMAKE_BUILD)
#include "../../../src/analysis_engine.cpp"
#include "../../../src/builtin_signatures.cpp"
#include "../../../src/confidence_resolver.cpp"
#include "../../../src/default_analysis.cpp"
#include "../../../src/json_trace_adapter.cpp"
#include "../../../src/json_trace_loader.cpp"
#include "../../../src/normalized_trace.cpp"
#include "../../../src/report_engine.cpp"
#include "../../../src/signature_result.cpp"
#include "../../../src/signature_utils.cpp"
#include "../../../src/spike_detector.cpp"
#include "../../../src/trace_adapter_registry.cpp"
#include "../../../src/version.cpp"
#endif
