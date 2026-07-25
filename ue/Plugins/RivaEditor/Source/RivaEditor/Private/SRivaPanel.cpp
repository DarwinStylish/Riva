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
#include "HAL/PlatformApplicationMisc.h"
#if defined(RIVA_UBT_BUILD)
#include "Async/Async.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#endif

#define LOCTEXT_NAMESPACE "SRivaPanel"

DEFINE_LOG_CATEGORY_STATIC(LogRivaEditor, Log, All);

void SRivaPanel::Construct(const FArguments& InArgs)
{
    UE_LOG(LogRivaEditor, Log, TEXT("Constructing SRivaPanel dockable tab widget layout."));

    CurrentSampleIndex = 0;
    bSyncWithInsightsEnabled = true;
    FRivaTraceService::RegisterInsightsSelectionCallback([this](double StartMs, double EndMs) {
        OnInsightsTimeRangeSelected(StartMs, EndMs);
    });

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
                    .Padding(0.0f, 0.0f, 6.0f, 0.0f)
                    [
                        SNew(SButton)
                        .OnClicked(this, &SRivaPanel::OnExportJsonClicked)
                        .ToolTipText(LOCTEXT("ExportJsonTooltip", "Save the diagnostic report as structured JSON."))
                        [
                            SNew(STextBlock)
                            .Text(LOCTEXT("ExportJsonButton", "Export JSON..."))
                        ]
                    ]

                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .Padding(0.0f, 0.0f, 6.0f, 0.0f)
                    [
                        SNew(SCheckBox)
                        .IsChecked(ECheckBoxState::Checked)
                        .OnCheckStateChanged(this, &SRivaPanel::OnSyncInsightsToggled)
                        .ToolTipText(LOCTEXT("SyncInsightsTooltip", "Toggle bidirectional time range synchronization with Unreal Insights."))
                        [
                            SNew(STextBlock)
                            .Text(LOCTEXT("SyncInsightsToggle", "Sync Insights"))
                        ]
                    ]

                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    [
                        SNew(SButton)
                        .OnClicked(this, &SRivaPanel::OnSimulateInsightsSyncClicked)
                        .ToolTipText(LOCTEXT("SimulateSyncTooltip", "Simulate receiving an inbound time range selection from Unreal Insights."))
                        [
                            SNew(STextBlock)
                            .Text(LOCTEXT("SimulateSyncButton", "Simulate Sync"))
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
                        .Padding(0.0f, 0.0f, 0.0f, 12.0f)
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .Padding(0.0f, 0.0f, 12.0f, 0.0f)
                            .VAlign(VAlign_Center)
                            [
                                SAssignNew(DetailsTimeWindowText, STextBlock)
                                .Text(LOCTEXT("DefaultTimeWindow", ""))
                                .ColorAndOpacity(FSlateColor::UseSubduedForeground())
                            ]
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .Padding(0.0f, 0.0f, 4.0f, 0.0f)
                            [
                                SNew(SButton)
                                .Text(LOCTEXT("CopyTimeWindowBtn", "Copy Time Window"))
                                .OnClicked(this, &SRivaPanel::OnCopyTimeWindowClicked)
                            ]
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            [
                                SNew(SButton)
                                .Text(LOCTEXT("CopySummaryBtn", "Copy Summary"))
                                .OnClicked(this, &SRivaPanel::OnCopySummaryClicked)
                            ]
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
    Finding1->StartTimeMs = 32.0;
    Finding1->EndTimeMs = 84.0;
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
    Finding2->StartTimeMs = 32.0;
    Finding2->EndTimeMs = 77.0;
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
    SelectedFinding = InItem;

    if (InItem.IsValid())
    {
        DetailsTitleText->SetText(InItem->Title);
        DetailsTimeWindowText->SetText(InItem->TimeWindow);
        DetailsContentText->SetText(InItem->DetailedReport);
        StatusBarText->SetText(FText::Format(LOCTEXT("StatusSelected", "Status: Inspecting finding — {0}"), InItem->Title));

        if (bSyncWithInsightsEnabled)
        {
            FRivaTraceService::BroadcastTimeRangeSelection(InItem->StartTimeMs, InItem->EndTimeMs);
        }
    }
    else
    {
        DetailsTitleText->SetText(LOCTEXT("DefaultDetailsTitle", "Diagnostic Evidence & Actionable Guidance"));
        DetailsTimeWindowText->SetText(FText::GetEmpty());
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
        const bool bSuccess = FRivaTraceService::LoadAndAnalyzeTrace(TracePath, ResultFindings, ErrorMsg);

        Async(EAsyncExecution::TaskGraphMainThread, [this, bSuccess, ResultFindings, ErrorMsg]() {
            OnAnalysisCompleted(bSuccess, ResultFindings, ErrorMsg);
        });
    });
#else
    TArray<FRivaUiFinding> ResultFindings;
    FString ErrorMsg;
    const bool bSuccess = FRivaTraceService::LoadAndAnalyzeTrace(TracePath, ResultFindings, ErrorMsg);
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
        TEXT("spike_shader_compile.json"),
        TEXT("spike_pso_miss.json"),
        TEXT("spike_streaming_io.json"),
        TEXT("sample_session.utrace")
    };

    CurrentSampleIndex = (CurrentSampleIndex + 1) % SampleTraces.Num();
    const FString SelectedSample = SampleTraces[CurrentSampleIndex];
    const FString FullPath = FString::Printf(TEXT("../../../samples/%s"), *SelectedSample);

    RunAsyncAnalysis(FullPath);
    return FReply::Handled();
}

FReply SRivaPanel::OnAnalyzeClicked()
{
    UE_LOG(LogRivaEditor, Log, TEXT("Analyze action triggered. Running analysis on default sample session."));
    const FString DefaultPath = TEXT("../../../samples/sample_session.utrace");
    RunAsyncAnalysis(DefaultPath);
    return FReply::Handled();
}

FReply SRivaPanel::OnCopySummaryClicked()
{
    if (SelectedFinding.IsValid())
    {
        FPlatformApplicationMisc::ClipboardCopy(*SelectedFinding->DetailedReport.ToString());
        StatusBarText->SetText(LOCTEXT("StatusCopiedSummary", "Status: Diagnostic summary copied to clipboard."));
    }
    return FReply::Handled();
}

FReply SRivaPanel::OnCopyTimeWindowClicked()
{
    if (SelectedFinding.IsValid())
    {
        FPlatformApplicationMisc::ClipboardCopy(*SelectedFinding->TimeWindow.ToString());
        StatusBarText->SetText(LOCTEXT("StatusCopiedTimeWindow", "Status: Time window copied to clipboard."));
    }
    return FReply::Handled();
}

FReply SRivaPanel::OnExportMarkdownClicked()
{
    UE_LOG(LogRivaEditor, Log, TEXT("Export Markdown action triggered."));
#if defined(RIVA_UBT_BUILD)
    if (IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get())
    {
        TArray<FString> OutFileNames;
        if (DesktopPlatform->SaveFileDialog(nullptr, LOCTEXT("SaveMarkdownTitle", "Export Riva Markdown Report").ToString(), FPaths::ProjectDir(), TEXT("RivaReport.md"), TEXT("Markdown Files (*.md)|*.md"), EFileDialogFlags::None, OutFileNames) && OutFileNames.Num() > 0)
        {
            FString ErrorMessage;
            if (FRivaTraceService::ExportLastAnalysisToMarkdown(OutFileNames[0], ErrorMessage))
            {
                StatusBarText->SetText(LOCTEXT("StatusExportMdSuccess", "Status: Markdown report exported successfully."));
            }
            else
            {
                StatusBarText->SetText(FText::Format(LOCTEXT("StatusExportMdError", "Status: Export failed — {0}"), FText::FromString(ErrorMessage)));
            }
        }
    }
#endif
    return FReply::Handled();
}

FReply SRivaPanel::OnExportJsonClicked()
{
    UE_LOG(LogRivaEditor, Log, TEXT("Export JSON action triggered."));
#if defined(RIVA_UBT_BUILD)
    if (IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get())
    {
        TArray<FString> OutFileNames;
        if (DesktopPlatform->SaveFileDialog(nullptr, LOCTEXT("SaveJsonTitle", "Export Riva JSON Report").ToString(), FPaths::ProjectDir(), TEXT("RivaReport.json"), TEXT("JSON Files (*.json)|*.json"), EFileDialogFlags::None, OutFileNames) && OutFileNames.Num() > 0)
        {
            FString ErrorMessage;
            if (FRivaTraceService::ExportLastAnalysisToJson(OutFileNames[0], ErrorMessage))
            {
                StatusBarText->SetText(LOCTEXT("StatusExportJsonSuccess", "Status: JSON report exported successfully."));
            }
            else
            {
                StatusBarText->SetText(FText::Format(LOCTEXT("StatusExportJsonError", "Status: Export failed — {0}"), FText::FromString(ErrorMessage)));
            }
        }
    }
#endif
    return FReply::Handled();
}

SRivaPanel::~SRivaPanel()
{
    FRivaTraceService::UnregisterInsightsSelectionCallback();
}

void SRivaPanel::OnSyncInsightsToggled(ECheckBoxState NewState)
{
    bSyncWithInsightsEnabled = (NewState == ECheckBoxState::Checked);
    UE_LOG(LogRivaEditor, Log, TEXT("Unreal Insights bidirectional sync toggled: %s"), bSyncWithInsightsEnabled ? TEXT("ON") : TEXT("OFF"));
    if (StatusBarText.IsValid())
    {
        StatusBarText->SetText(bSyncWithInsightsEnabled
            ? LOCTEXT("StatusSyncOn", "Status: Unreal Insights synchronization enabled — time windows linked.")
            : LOCTEXT("StatusSyncOff", "Status: Unreal Insights synchronization disabled."));
    }
}

FReply SRivaPanel::OnSimulateInsightsSyncClicked()
{
    UE_LOG(LogRivaEditor, Log, TEXT("Simulate Sync action triggered."));
    FRivaTraceService::SimulateInsightsSelection(32.0, 84.0);
    return FReply::Handled();
}

void SRivaPanel::OnInsightsTimeRangeSelected(double StartMs, double EndMs)
{
    if (!bSyncWithInsightsEnabled)
    {
        return;
    }

    UE_LOG(LogRivaEditor, Log, TEXT("Inbound Insights time range selection received: %.2f ms - %.2f ms"), StartMs, EndMs);
    for (const TSharedPtr<FRivaUiFinding>& Finding : FindingsList)
    {
        if (Finding.IsValid() && Finding->StartTimeMs <= EndMs && Finding->EndTimeMs >= StartMs)
        {
            if (FindingsListView.IsValid())
            {
                FindingsListView->SetSelection(Finding);
                if (StatusBarText.IsValid())
                {
                    StatusBarText->SetText(FText::Format(LOCTEXT("StatusSyncReceived", "Status: Synchronized from Unreal Insights — selected '{0}'."), Finding->Title));
                }
            }
            break;
        }
    }
}

#undef LOCTEXT_NAMESPACE
