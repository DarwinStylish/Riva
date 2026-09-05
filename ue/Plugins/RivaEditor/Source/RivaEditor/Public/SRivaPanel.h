#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"

struct FRivaUiFinding {
  FText Title;
  FText Role;
  FText Confidence;
  FText TimeWindow;
  FText DetailedReport;
  double StartTimeMs{0.0};
  double EndTimeMs{0.0};
};

struct FRivaUiBudgetStatus {
  bool bConfigured{false};
  bool bBreached{false};
  TArray<FString> BreachedMetrics;
};

class SRivaPanel : public SCompoundWidget {
 public:
  SLATE_BEGIN_ARGS(SRivaPanel) {}
  SLATE_END_ARGS()

  void Construct(const FArguments& InArgs);
  void OnAnalysisCompleted(uint64 RequestId, bool bSuccess,
                           const TArray<FRivaUiFinding>& InFindings,
                           const FRivaUiBudgetStatus& BudgetStatus, const FString& ErrorMessage);

 private:
  TSharedRef<ITableRow> OnGenerateFindingRow(TSharedPtr<FRivaUiFinding> InItem,
                                             const TSharedRef<STableViewBase>& OwnerTable);
  void OnFindingSelectionChanged(TSharedPtr<FRivaUiFinding> InItem, ESelectInfo::Type SelectInfo);

  FReply OnOpenTraceClicked();
  FReply OnAnalyzeClicked();
  FReply OnExportMarkdownClicked();
  FReply OnExportJsonClicked();
  FReply OnCopySummaryClicked();
  FReply OnCopyTimeWindowClicked();
  void RunAsyncAnalysis(const FString& TracePath);

  TArray<TSharedPtr<FRivaUiFinding>> FindingsList;
  TSharedPtr<SListView<TSharedPtr<FRivaUiFinding>>> FindingsListView;
  TSharedPtr<class STextBlock> DetailsTitleText;
  TSharedPtr<class STextBlock> DetailsTimeWindowText;
  TSharedPtr<class STextBlock> DetailsContentText;
  TSharedPtr<class STextBlock> StatusBarText;
  TSharedPtr<class STextBlock> BudgetStatusText;
  TSharedPtr<FRivaUiFinding> SelectedFinding;

  FString LoadedTracePath;
  uint64 ActiveAnalysisRequestId{0};
};
