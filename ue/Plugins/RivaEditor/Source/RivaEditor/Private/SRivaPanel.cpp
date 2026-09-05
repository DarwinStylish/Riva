#include "SRivaPanel.h"

#include "Async/Async.h"
#include "DesktopPlatformModule.h"
#include "HAL/PlatformApplicationMisc.h"
#include "IDesktopPlatform.h"
#include "RivaTraceService.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SRivaPanel"

DEFINE_LOG_CATEGORY_STATIC(LogRivaEditor, Log, All);

void SRivaPanel::Construct(const FArguments& InArgs) {
  UE_LOG(LogRivaEditor, Log, TEXT("Constructing SRivaPanel dockable tab widget layout."));

  LoadedTracePath = TEXT("");
  ActiveAnalysisRequestId = 0;
  FRivaTraceService::InvalidateAnalysisResults();

  ChildSlot
      [SNew(SBorder)
           .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
           .Padding(FMargin(6.0f))
               [SNew(SVerticalBox)

                // Top Section: Toolbar
                +
                SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
                    [SNew(SBorder)
                         .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
                         .Padding(FMargin(4.0f))
                             [SNew(SHorizontalBox)

                              + SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
                                    [SNew(SButton)
                                         .OnClicked(this, &SRivaPanel::OnOpenTraceClicked)
                                         .ToolTipText(LOCTEXT(
                                             "OpenTraceTooltip",
                                             "Select an Unreal Insights trace file or JSON export "
                                             "for analysis."))[SNew(STextBlock)
                                                                   .Text(LOCTEXT("OpenTraceButton",
                                                                                 "Open Trace..."))]]

                              + SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
                                    [SNew(SButton)
                                         .OnClicked(this, &SRivaPanel::OnAnalyzeClicked)
                                         .ToolTipText(LOCTEXT("AnalyzeTooltip",
                                                              "Run deterministic performance hitch "
                                                              "analysis on the loaded trace."))
                                             [SNew(STextBlock)
                                                  .Text(LOCTEXT("AnalyzeButton", "Analyze"))]]

                              + SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
                                    [SNew(SButton)
                                         .OnClicked(this, &SRivaPanel::OnExportMarkdownClicked)
                                         .ToolTipText(LOCTEXT("ExportMarkdownTooltip",
                                                              "Save the diagnostic report as "
                                                              "Markdown."))
                                             [SNew(STextBlock)
                                                  .Text(LOCTEXT("ExportMarkdownButton",
                                                                "Export Markdown..."))]]

                              + SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
                                    [SNew(SButton)
                                         .OnClicked(this, &SRivaPanel::OnExportJsonClicked)
                                         .ToolTipText(LOCTEXT("ExportJsonTooltip",
                                                              "Save the diagnostic report as "
                                                              "structured JSON."))
                                             [SNew(STextBlock)
                                                  .Text(LOCTEXT("ExportJsonButton",
                                                                "Export JSON..."))]]

                              // Budget Status indicator
                              +
                              SHorizontalBox::Slot()
                                  .FillWidth(1.0f)
                                  .HAlign(HAlign_Right)
                                  .VAlign(VAlign_Center)
                                      [SAssignNew(BudgetStatusText, STextBlock)
                                           .Text(LOCTEXT("BudgetStatusInit", "Budget: Unknown"))
                                           .ColorAndOpacity(FSlateColor::UseSubduedForeground())]]]

                // Middle Section: Split View
                +
                SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 0.0f, 0.0f, 6.0f)
                    [SNew(SSplitter).Orientation(Orient_Horizontal)

                     // Left Pane: Findings List
                     + SSplitter::Slot().Value(0.35f)
                           [SNew(SBorder)
                                .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
                                .Padding(FMargin(4.0f))
                                    [SNew(SVerticalBox)

                                     + SVerticalBox::Slot().AutoHeight().Padding(
                                           4.0f, 4.0f, 4.0f,
                                           6.0f)[SNew(STextBlock)
                                                     .Text(LOCTEXT("FindingsListHeader",
                                                                   "Detected Hitches & Stalls"))
                                                     .Font(FAppStyle::GetFontStyle("HeadingSmall"))]

                                     + SVerticalBox::Slot().FillHeight(1.0f)
                                           [SAssignNew(FindingsListView,
                                                       SListView<TSharedPtr<FRivaUiFinding>>)
                                                .ListItemsSource(&FindingsList)
                                                .OnGenerateRow(
                                                    this, &SRivaPanel::OnGenerateFindingRow)
                                                .OnSelectionChanged(
                                                    this, &SRivaPanel::OnFindingSelectionChanged)
                                                .SelectionMode(ESelectionMode::Single)]]]

                     // Right Pane: Details Pane
                     + SSplitter::Slot().Value(0.65f)
                           [SNew(SBorder)
                                .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
                                .Padding(FMargin(8.0f))
                                    [SNew(SScrollBox)

                                     + SScrollBox::Slot().Padding(0.0f, 0.0f, 0.0f, 8.0f)
                                           [SAssignNew(DetailsTitleText, STextBlock)
                                                .Text(LOCTEXT("DefaultDetailsTitle",
                                                              "Diagnostic Evidence & Actionable "
                                                              "Guidance"))
                                                .Font(FAppStyle::GetFontStyle("HeadingSmall"))]

                                     + SScrollBox::Slot().Padding(0.0f, 0.0f, 0.0f, 12.0f)
                                           [SNew(SHorizontalBox) +
                                            SHorizontalBox::Slot()
                                                .AutoWidth()
                                                .Padding(0.0f, 0.0f, 12.0f, 0.0f)
                                                .VAlign(VAlign_Center)
                                                    [SAssignNew(DetailsTimeWindowText, STextBlock)
                                                         .Text(LOCTEXT("DefaultTimeWindow", ""))
                                                         .ColorAndOpacity(
                                                             FSlateColor::UseSubduedForeground())] +
                                            SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f,
                                                                                       4.0f, 0.0f)
                                                [SNew(SButton)
                                                     .Text(LOCTEXT("CopyTimeWindowBtn",
                                                                   "Copy Time Window"))
                                                     .OnClicked(
                                                         this,
                                                         &SRivaPanel::OnCopyTimeWindowClicked)] +
                                            SHorizontalBox::Slot().AutoWidth()
                                                [SNew(SButton)
                                                     .Text(LOCTEXT("CopySummaryBtn",
                                                                   "Copy "
                                                                   "Summary"))
                                                     .OnClicked(this,
                                                                &SRivaPanel::OnCopySummaryClicked)]]

                                     + SScrollBox::Slot()
                                           [SAssignNew(DetailsContentText, STextBlock)
                                                .Text(LOCTEXT(
                                                    "DefaultDetailsContent",
                                                    "Select a detected hitch from the findings "
                                                    "list on the left to inspect its calibrated "
                                                    "confidence, supporting runtime evidence, and "
                                                    "Unreal Insights confirmation guidance."))
                                                .AutoWrapText(true)]]]]

                // Bottom Section: Status Bar
                + SVerticalBox::Slot().AutoHeight()
                      [SNew(SBorder)
                           .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
                           .Padding(FMargin(4.0f, 2.0f))
                               [SAssignNew(StatusBarText, STextBlock)
                                    .Text(LOCTEXT("StatusReady",
                                                  "Status: Ready. Please open a trace file."))
                                    .ColorAndOpacity(FSlateColor::UseSubduedForeground())]]]];
}

TSharedRef<ITableRow> SRivaPanel::OnGenerateFindingRow(
    TSharedPtr<FRivaUiFinding> InItem, const TSharedRef<STableViewBase>& OwnerTable) {
  return SNew(STableRow<TSharedPtr<FRivaUiFinding>>, OwnerTable)
      .Padding(FMargin(4.0f))
          [SNew(SVerticalBox)

           + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock)
                                                   .Text(InItem->Title)
                                                   .Font(FAppStyle::GetFontStyle("NormalFontBold"))]

           +
           SVerticalBox::Slot().AutoHeight().Padding(
               0.0f, 2.0f, 0.0f,
               0.0f)[SNew(SHorizontalBox)

                     + SHorizontalBox::Slot().AutoWidth().Padding(
                           0.0f, 0.0f, 8.0f,
                           0.0f)[SNew(STextBlock)
                                     .Text(InItem->Role)
                                     .ColorAndOpacity(FSlateColor::UseSubduedForeground())]

                     + SHorizontalBox::Slot()
                           .AutoWidth()[SNew(STextBlock)
                                            .Text(InItem->Confidence)
                                            .ColorAndOpacity(FSlateColor::UseSubduedForeground())]]

           + SVerticalBox::Slot().AutoHeight().Padding(
                 0.0f, 2.0f, 0.0f,
                 0.0f)[SNew(STextBlock)
                           .Text(InItem->TimeWindow)
                           .ColorAndOpacity(FSlateColor::UseSubduedForeground())]];
}

void SRivaPanel::OnFindingSelectionChanged(TSharedPtr<FRivaUiFinding> InItem,
                                           ESelectInfo::Type SelectInfo) {
  SelectedFinding = InItem;

  if (InItem.IsValid()) {
    DetailsTitleText->SetText(InItem->Title);
    DetailsTimeWindowText->SetText(InItem->TimeWindow);
    DetailsContentText->SetText(InItem->DetailedReport);
    StatusBarText->SetText(FText::Format(
        LOCTEXT("StatusSelected", "Status: Inspecting finding — {0}"), InItem->Title));

  } else {
    DetailsTitleText->SetText(
        LOCTEXT("DefaultDetailsTitle", "Diagnostic Evidence & Actionable Guidance"));
    DetailsTimeWindowText->SetText(FText::GetEmpty());
    DetailsContentText->SetText(LOCTEXT(
        "DefaultDetailsContent",
        "Select a detected hitch from the findings list on the left to inspect its calibrated "
        "confidence, supporting runtime evidence, and Unreal Insights confirmation guidance."));
    StatusBarText->SetText(LOCTEXT("StatusReady", "Status: Ready. Please open a trace file."));
  }
}

void SRivaPanel::RunAsyncAnalysis(const FString& TracePath) {
  UE_LOG(LogRivaEditor, Log, TEXT("Initiating async trace analysis for path: %s"), *TracePath);
  const uint64 RequestId = FRivaTraceService::BeginAnalysisRequest();
  ActiveAnalysisRequestId = RequestId;
  FindingsList.Empty();
  SelectedFinding.Reset();
  if (FindingsListView.IsValid()) {
    FindingsListView->RequestListRefresh();
  }
  DetailsTitleText->SetText(LOCTEXT("AnalysisPendingTitle", "Analysis in progress"));
  DetailsTimeWindowText->SetText(FText::GetEmpty());
  DetailsContentText->SetText(
      LOCTEXT("AnalysisPendingDetails", "Results will appear when analysis completes."));
  BudgetStatusText->SetText(LOCTEXT("BudgetPending", "Budget: Pending"));
  BudgetStatusText->SetColorAndOpacity(FSlateColor::UseSubduedForeground());
  StatusBarText->SetText(
      LOCTEXT("StatusAnalyzing",
              "Status: Running async deterministic trace analysis on background thread pool..."));

  const TWeakPtr<SRivaPanel> WeakThis = SharedThis(this);
  Async(EAsyncExecution::ThreadPool, [WeakThis, TracePath, RequestId]() {
    TArray<FRivaUiFinding> ResultFindings;
    FRivaUiBudgetStatus BudgetStatus;
    FString ErrorMsg;
    const bool bSuccess = FRivaTraceService::LoadAndAnalyzeTrace(
        RequestId, TracePath, ResultFindings, BudgetStatus, ErrorMsg);

    AsyncTask(ENamedThreads::GameThread, [WeakThis, RequestId, bSuccess, ResultFindings,
                                          BudgetStatus, ErrorMsg]() {
      if (const TSharedPtr<SRivaPanel> Panel = WeakThis.Pin()) {
        Panel->OnAnalysisCompleted(RequestId, bSuccess, ResultFindings, BudgetStatus, ErrorMsg);
      }
    });
  });
}

void SRivaPanel::OnAnalysisCompleted(uint64 RequestId, bool bSuccess,
                                     const TArray<FRivaUiFinding>& InFindings,
                                     const FRivaUiBudgetStatus& BudgetStatus,
                                     const FString& ErrorMessage) {
  if (RequestId != ActiveAnalysisRequestId) {
    UE_LOG(LogRivaEditor, Verbose,
           TEXT("Ignoring stale analysis completion for request %llu (active request: %llu)."),
           static_cast<unsigned long long>(RequestId),
           static_cast<unsigned long long>(ActiveAnalysisRequestId));
    return;
  }
  ActiveAnalysisRequestId = 0;

  if (bSuccess) {
    FindingsList.Empty();
    for (const FRivaUiFinding& Finding : InFindings) {
      FindingsList.Add(MakeShared<FRivaUiFinding>(Finding));
    }
    if (FindingsListView.IsValid()) {
      FindingsListView->RequestListRefresh();
    }
    StatusBarText->SetText(
        FText::Format(LOCTEXT("StatusSuccess", "Status: Analysis complete — {0} hitches detected."),
                      FText::AsNumber(FindingsList.Num())));

    if (FindingsList.Num() > 0 && FindingsListView.IsValid()) {
      FindingsListView->SetSelection(FindingsList[0]);
    }

    if (BudgetStatusText.IsValid()) {
      if (!BudgetStatus.bConfigured) {
        BudgetStatusText->SetText(LOCTEXT("BudgetNotConfigured", "Budget: Not configured"));
        BudgetStatusText->SetColorAndOpacity(FSlateColor::UseSubduedForeground());
      } else if (BudgetStatus.bBreached) {
        BudgetStatusText->SetText(
            FText::Format(LOCTEXT("BudgetBreached", "Budget: BREACHED ({0} metrics)"),
                          FText::AsNumber(BudgetStatus.BreachedMetrics.Num())));
        BudgetStatusText->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
      } else {
        BudgetStatusText->SetText(LOCTEXT("BudgetOk", "Budget: OK"));
        BudgetStatusText->SetColorAndOpacity(FSlateColor(FLinearColor::Green));
      }
    }
  } else {
    StatusBarText->SetText(FText::Format(LOCTEXT("StatusError", "Status: Analysis failed — {0}"),
                                         FText::FromString(ErrorMessage)));
  }
}

FReply SRivaPanel::OnOpenTraceClicked() {
  UE_LOG(LogRivaEditor, Log, TEXT("Open Trace action triggered."));
  if (IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get()) {
    TArray<FString> OutFileNames;
    if (DesktopPlatform->OpenFileDialog(
            nullptr, LOCTEXT("OpenTraceTitle", "Open Riva Trace").ToString(), FPaths::ProjectDir(),
            TEXT(""), TEXT("Trace Files (*.utrace;*.json)|*.utrace;*.json"), EFileDialogFlags::None,
            OutFileNames) &&
        OutFileNames.Num() > 0) {
      LoadedTracePath = OutFileNames[0];
      ActiveAnalysisRequestId = 0;
      FRivaTraceService::InvalidateAnalysisResults();
      FindingsList.Empty();
      SelectedFinding.Reset();
      if (FindingsListView.IsValid()) {
        FindingsListView->RequestListRefresh();
      }
      DetailsTitleText->SetText(
          LOCTEXT("TraceLoadedDetailsTitle", "Trace loaded; analysis not started"));
      DetailsTimeWindowText->SetText(FText::GetEmpty());
      DetailsContentText->SetText(
          LOCTEXT("TraceLoadedDetails", "Select Analyze to generate findings for this trace."));
      BudgetStatusText->SetText(LOCTEXT("BudgetUnknown", "Budget: Unknown"));
      BudgetStatusText->SetColorAndOpacity(FSlateColor::UseSubduedForeground());
      StatusBarText->SetText(
          FText::Format(LOCTEXT("StatusTraceLoaded", "Status: Trace loaded — {0}"),
                        FText::FromString(FPaths::GetCleanFilename(LoadedTracePath))));
    }
  }
  return FReply::Handled();
}

FReply SRivaPanel::OnAnalyzeClicked() {
  UE_LOG(LogRivaEditor, Log, TEXT("Analyze action triggered."));
  if (LoadedTracePath.IsEmpty()) {
    StatusBarText->SetText(
        LOCTEXT("StatusNoTrace", "Status: Error — No trace file loaded to analyze."));
    return FReply::Handled();
  }
  RunAsyncAnalysis(LoadedTracePath);
  return FReply::Handled();
}

FReply SRivaPanel::OnCopySummaryClicked() {
  if (SelectedFinding.IsValid()) {
    FPlatformApplicationMisc::ClipboardCopy(*SelectedFinding->DetailedReport.ToString());
    StatusBarText->SetText(
        LOCTEXT("StatusCopiedSummary", "Status: Diagnostic summary copied to clipboard."));
  }
  return FReply::Handled();
}

FReply SRivaPanel::OnCopyTimeWindowClicked() {
  if (SelectedFinding.IsValid()) {
    FPlatformApplicationMisc::ClipboardCopy(*SelectedFinding->TimeWindow.ToString());
    StatusBarText->SetText(
        LOCTEXT("StatusCopiedTimeWindow", "Status: Time window copied to clipboard."));
  }
  return FReply::Handled();
}

FReply SRivaPanel::OnExportMarkdownClicked() {
  UE_LOG(LogRivaEditor, Log, TEXT("Export Markdown action triggered."));
  if (IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get()) {
    TArray<FString> OutFileNames;
    if (DesktopPlatform->SaveFileDialog(
            nullptr, LOCTEXT("SaveMarkdownTitle", "Export Riva Markdown Report").ToString(),
            FPaths::ProjectDir(), TEXT("RivaReport.md"), TEXT("Markdown Files (*.md)|*.md"),
            EFileDialogFlags::None, OutFileNames) &&
        OutFileNames.Num() > 0) {
      FString ErrorMessage;
      if (FRivaTraceService::ExportLastAnalysisToMarkdown(OutFileNames[0], ErrorMessage)) {
        StatusBarText->SetText(
            LOCTEXT("StatusExportMdSuccess", "Status: Markdown report exported successfully."));
      } else {
        StatusBarText->SetText(
            FText::Format(LOCTEXT("StatusExportMdError", "Status: Export failed — {0}"),
                          FText::FromString(ErrorMessage)));
      }
    }
  }
  return FReply::Handled();
}

FReply SRivaPanel::OnExportJsonClicked() {
  UE_LOG(LogRivaEditor, Log, TEXT("Export JSON action triggered."));
  if (IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get()) {
    TArray<FString> OutFileNames;
    if (DesktopPlatform->SaveFileDialog(
            nullptr, LOCTEXT("SaveJsonTitle", "Export Riva JSON Report").ToString(),
            FPaths::ProjectDir(), TEXT("RivaReport.json"), TEXT("JSON Files (*.json)|*.json"),
            EFileDialogFlags::None, OutFileNames) &&
        OutFileNames.Num() > 0) {
      FString ErrorMessage;
      if (FRivaTraceService::ExportLastAnalysisToJson(OutFileNames[0], ErrorMessage)) {
        StatusBarText->SetText(
            LOCTEXT("StatusExportJsonSuccess", "Status: JSON report exported successfully."));
      } else {
        StatusBarText->SetText(
            FText::Format(LOCTEXT("StatusExportJsonError", "Status: Export failed — {0}"),
                          FText::FromString(ErrorMessage)));
      }
    }
  }
  return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
