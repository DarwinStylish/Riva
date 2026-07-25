#include "SRivaPanel.h"
#include "RivaTraceService.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"
#include "Styling/AppStyle.h"
#if defined(RIVA_UBT_BUILD)
#include "Async/Async.h"
#endif

#define LOCTEXT_NAMESPACE "SRivaPanel"

DEFINE_LOG_CATEGORY_STATIC(LogRivaEditor, Log, All);

void SRivaPanel::Construct(const FArguments& InArgs)
{
    UE_LOG(LogRivaEditor, Log, TEXT("Constructing SRivaPanel dockable tab widget layout."));

    CurrentSampleIndex = 0;
    PopulateInitialState();

    ChildSlot
    [
        SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(FMargin(6.0f))
        [
            SNew(SVerticalBox)

            // Top Section: Toolbar
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 0.0f, 0.0f, 6.0f)
            [
                SNew(SBorder)
                .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
                .Padding(FMargin(4.0f))
                [
                    SNew(SHorizontalBox)

                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .Padding(0.0f, 0.0f, 6.0f, 0.0f)
                    [
                        SNew(SButton)
                        .OnClicked(this, &SRivaPanel::OnOpenTraceClicked)
                        .ToolTipText(LOCTEXT("OpenTraceTooltip", "Select an Unreal Insights trace file or JSON export for analysis."))
                        [
                            SNew(STextBlock)
                            .Text(LOCTEXT("OpenTraceButton", "Open Trace..."))
                        ]
                    ]

                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .Padding(0.0f, 0.0f, 6.0f, 0.0f)
                    [
                        SNew(SButton)
                        .OnClicked(this, &SRivaPanel::OnAnalyzeClicked)
                        .ToolTipText(LOCTEXT("AnalyzeTooltip", "Run deterministic performance hitch analysis on the loaded trace."))
                        [
                            SNew(STextBlock)
                            .Text(LOCTEXT("AnalyzeButton", "Analyze"))
                        ]
                    ]

                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .Padding(0.0f, 0.0f, 6.0f, 0.0f)
                    [
                        SNew(SButton)
                        .OnClicked(this, &SRivaPanel::OnExportMarkdownClicked)
                        .ToolTipText(LOCTEXT("ExportMarkdownTooltip", "Save the diagnostic report as Markdown."))
                        [
                            SNew(STextBlock)
                            .Text(LOCTEXT("ExportMarkdownButton", "Export Markdown..."))
                        ]
                    ]

                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    [
                        SNew(SButton)
                        .OnClicked(this, &SRivaPanel::OnExportJsonClicked)
                        .ToolTipText(LOCTEXT("ExportJsonTooltip", "Save the diagnostic report as structured JSON."))
                        [
                            SNew(STextBlock)
                            .Text(LOCTEXT("ExportJsonButton", "Export JSON..."))
                        ]
                    ]
                ]
            ]

            // Middle Section: Split View
            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            .Padding(0.0f, 0.0f, 0.0f, 6.0f)
            [
                SNew(SSplitter)
                .Orientation(Orient_Horizontal)

                // Left Pane: Findings List
                + SSplitter::Slot()
                .Value(0.35f)
                [
                    SNew(SBorder)
                    .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
                    .Padding(FMargin(4.0f))
                    [
                        SNew(SVerticalBox)

                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(4.0f, 4.0f, 4.0f, 6.0f)
                        [
                            SNew(STextBlock)
                            .Text(LOCTEXT("FindingsListHeader", "Detected Hitches & Stalls"))
                            .Font(FAppStyle::GetFontStyle("HeadingSmall"))
                        ]

                        + SVerticalBox::Slot()
                        .FillHeight(1.0f)
                        [
                            SAssignNew(FindingsListView, SListView<TSharedPtr<FRivaUiFinding>>)
                            .ListItemsSource(&FindingsList)
                            .OnGenerateRow(this, &SRivaPanel::OnGenerateFindingRow)
                            .OnSelectionChanged(this, &SRivaPanel::OnFindingSelectionChanged)
                            .SelectionMode(ESelectionMode::Single)
                        ]
                    ]
                ]

                // Right Pane: Details Pane
                + SSplitter::Slot()
                .Value(0.65f)
                [
                    SNew(SBorder)
                    .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
                    .Padding(FMargin(8.0f))
                    [
                        SNew(SScrollBox)

                        + SScrollBox::Slot()
                        .Padding(0.0f, 0.0f, 0.0f, 8.0f)
                        [
                            SAssignNew(DetailsTitleText, STextBlock)
                            .Text(LOCTEXT("DefaultDetailsTitle", "Diagnostic Evidence & Actionable Guidance"))
                            .Font(FAppStyle::GetFontStyle("HeadingSmall"))
                        ]

                        + SScrollBox::Slot()
                        [
                            SAssignNew(DetailsContentText, STextBlock)
                            .Text(LOCTEXT("DefaultDetailsContent", "Select a detected hitch from the findings list on the left to inspect its calibrated confidence, supporting runtime evidence, and Unreal Insights confirmation guidance."))
                            .AutoWrapText(true)
                        ]
                    ]
                ]
            ]

            // Bottom Section: Status Bar
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SBorder)
                .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
                .Padding(FMargin(4.0f, 2.0f))
                [
                    SAssignNew(StatusBarText, STextBlock)
                    .Text(LOCTEXT("StatusReady", "Status: Ready for trace analysis — 2 sample hitches loaded."))
                    .ColorAndOpacity(FSlateColor::UseSubduedForeground())
                ]
            ]
        ]
    ];
}

void SRivaPanel::PopulateInitialState()
{
    TSharedPtr<FRivaUiFinding> Finding1 = MakeShared<FRivaUiFinding>();
    Finding1->Title = LOCTEXT("Sample1Title", "Shader compilation stall");
    Finding1->Role = LOCTEXT("Sample1Role", "[Primary]");
    Finding1->Confidence = LOCTEXT("Sample1Conf", "Confidence: 89.0%");
    Finding1->TimeWindow = LOCTEXT("Sample1Time", "32.00 ms - 84.00 ms");
    Finding1->DetailedReport = LOCTEXT("Sample1Report",
        "Executive Summary:\n"
        "Primary finding selected by calibrated confidence and runtime shader worker evidence.\n\n"
        "Evidence Breakdown:\n"
        "- frame_ms: 52.00 ms\n"
        "- baseline_ms: 16.05 ms\n"
        "- ShaderCompileWorker blocked frame\n\n"
        "Actionable Guidance:\n"
        "1. Open Unreal Insights and navigate to the timing window.\n"
        "2. Verify shader compilation worker threads during the hitch.\n"
        "3. Precompile shaders or use pipeline state caching to mitigate runtime hitches.");

    TSharedPtr<FRivaUiFinding> Finding2 = MakeShared<FRivaUiFinding>();
    Finding2->Title = LOCTEXT("Sample2Title", "Streaming or IO stall");
    Finding2->Role = LOCTEXT("Sample2Role", "[Secondary]");
    Finding2->Confidence = LOCTEXT("Sample2Conf", "Confidence: 70.0%");
    Finding2->TimeWindow = LOCTEXT("Sample2Time", "32.00 ms - 77.00 ms");
    Finding2->DetailedReport = LOCTEXT("Sample2Report",
        "Executive Summary:\n"
        "Secondary finding detected concurrent with primary stall.\n\n"
        "Evidence Breakdown:\n"
        "- frame_ms: 45.00 ms\n"
        "- Asset loading Zen IO dispatcher stall\n\n"
        "Actionable Guidance:\n"
        "1. Inspect disk I/O and asset loading background threads.\n"
        "2. Ensure async loading queues are not saturated during critical gameplay transitions.");

    FindingsList.Add(Finding1);
    FindingsList.Add(Finding2);
}

TSharedRef<ITableRow> SRivaPanel::OnGenerateFindingRow(TSharedPtr<FRivaUiFinding> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
    return SNew(STableRow<TSharedPtr<FRivaUiFinding>>, OwnerTable)
        .Padding(FMargin(4.0f))
        [
            SNew(SVerticalBox)

            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(STextBlock)
                .Text(InItem->Title)
                .Font(FAppStyle::GetFontStyle("NormalFontBold"))
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 2.0f, 0.0f, 0.0f)
            [
                SNew(SHorizontalBox)

                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(0.0f, 0.0f, 8.0f, 0.0f)
                [
                    SNew(STextBlock)
                    .Text(InItem->Role)
                    .ColorAndOpacity(FSlateColor::UseSubduedForeground())
                ]

                + SHorizontalBox::Slot()
                .AutoWidth()
                [
                    SNew(STextBlock)
                    .Text(InItem->Confidence)
                    .ColorAndOpacity(FSlateColor::UseSubduedForeground())
                ]
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 2.0f, 0.0f, 0.0f)
            [
                SNew(STextBlock)
                .Text(InItem->TimeWindow)
                .ColorAndOpacity(FSlateColor::UseSubduedForeground())
            ]
        ];
}

void SRivaPanel::OnFindingSelectionChanged(TSharedPtr<FRivaUiFinding> InItem, ESelectInfo::Type SelectInfo)
{
    if (InItem.IsValid())
    {
        DetailsTitleText->SetText(InItem->Title);
        DetailsContentText->SetText(InItem->DetailedReport);
        StatusBarText->SetText(FText::Format(LOCTEXT("StatusSelected", "Status: Inspecting finding — {0}"), InItem->Title));
    }
    else
    {
        DetailsTitleText->SetText(LOCTEXT("DefaultDetailsTitle", "Diagnostic Evidence & Actionable Guidance"));
        DetailsContentText->SetText(LOCTEXT("DefaultDetailsContent", "Select a detected hitch from the findings list on the left to inspect its calibrated confidence, supporting runtime evidence, and Unreal Insights confirmation guidance."));
        StatusBarText->SetText(LOCTEXT("StatusReady", "Status: Ready for trace analysis — 2 sample hitches loaded."));
    }
}

void SRivaPanel::RunAsyncAnalysis(const FString& TracePath)
{
    UE_LOG(LogRivaEditor, Log, TEXT("Initiating async trace analysis for path: %s"), *TracePath);
    StatusBarText->SetText(LOCTEXT("StatusAnalyzing", "Status: Running async deterministic trace analysis on background thread pool..."));

#if defined(RIVA_UBT_BUILD)
    Async(EAsyncExecution::ThreadPool, [this, TracePath]() {
        TArray<FRivaUiFinding> ResultFindings;
        FString ErrorMsg;
        const bool bSuccess = FRivaTraceService::LoadAndAnalyzeJsonTrace(TracePath, ResultFindings, ErrorMsg);

        Async(EAsyncExecution::TaskGraphMainThread, [this, bSuccess, ResultFindings, ErrorMsg]() {
            OnAnalysisCompleted(bSuccess, ResultFindings, ErrorMsg);
        });
    });
#else
    TArray<FRivaUiFinding> ResultFindings;
    FString ErrorMsg;
    const bool bSuccess = FRivaTraceService::LoadAndAnalyzeJsonTrace(TracePath, ResultFindings, ErrorMsg);
    OnAnalysisCompleted(bSuccess, ResultFindings, ErrorMsg);
#endif
}

void SRivaPanel::OnAnalysisCompleted(bool bSuccess, const TArray<FRivaUiFinding>& InFindings, const FString& ErrorMessage)
{
    if (bSuccess)
    {
        FindingsList.Empty();
        for (const FRivaUiFinding& Finding : InFindings)
        {
            FindingsList.Add(MakeShared<FRivaUiFinding>(Finding));
        }
        if (FindingsListView.IsValid())
        {
            FindingsListView->RequestListRefresh();
        }
        StatusBarText->SetText(FText::Format(LOCTEXT("StatusSuccess", "Status: Analysis complete — {0} hitches detected."), FText::AsNumber(FindingsList.Num())));

        if (FindingsList.Num() > 0 && FindingsListView.IsValid())
        {
            FindingsListView->SetSelection(FindingsList[0]);
        }
    }
    else
    {
        StatusBarText->SetText(FText::Format(LOCTEXT("StatusError", "Status: Analysis failed — {0}"), FText::FromString(ErrorMessage)));
    }
}

FReply SRivaPanel::OnOpenTraceClicked()
{
    UE_LOG(LogRivaEditor, Log, TEXT("Open Trace action triggered. Cycling through sample trace datasets."));
    const TArray<FString> SampleTraces = {
        TEXT("trace_01_shader_compile.json"),
        TEXT("trace_02_pso_miss.json"),
        TEXT("trace_03_streaming_io.json")
    };

    CurrentSampleIndex = (CurrentSampleIndex + 1) % SampleTraces.Num();
    const FString SelectedSample = SampleTraces[CurrentSampleIndex];
    const FString FullPath = FString::Printf(TEXT("../../../samples/%s"), *SelectedSample);

    RunAsyncAnalysis(FullPath);
    return FReply::Handled();
}

FReply SRivaPanel::OnAnalyzeClicked()
{
    UE_LOG(LogRivaEditor, Log, TEXT("Analyze action triggered. Running analysis on default sample trace."));
    const FString DefaultPath = TEXT("../../../samples/trace_01_shader_compile.json");
    RunAsyncAnalysis(DefaultPath);
    return FReply::Handled();
}

FReply SRivaPanel::OnExportMarkdownClicked()
{
    UE_LOG(LogRivaEditor, Log, TEXT("Export Markdown action triggered."));
    StatusBarText->SetText(LOCTEXT("StatusExportMdClicked", "Status: Markdown report export initiated."));
    return FReply::Handled();
}

FReply SRivaPanel::OnExportJsonClicked()
{
    UE_LOG(LogRivaEditor, Log, TEXT("Export JSON action triggered."));
    StatusBarText->SetText(LOCTEXT("StatusExportJsonClicked", "Status: JSON report export initiated."));
    return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
