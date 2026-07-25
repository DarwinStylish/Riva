#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"

struct FRivaUiFinding
{
    FText Title;
    FText Role;
    FText Confidence;
    FText TimeWindow;
    FText DetailedReport;
};

class SRivaPanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SRivaPanel) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    TSharedRef<ITableRow> OnGenerateFindingRow(TSharedPtr<FRivaUiFinding> InItem, const TSharedRef<STableViewBase>& OwnerTable);
    void OnFindingSelectionChanged(TSharedPtr<FRivaUiFinding> InItem, ESelectInfo::Type SelectInfo);

    FReply OnOpenTraceClicked();
    FReply OnAnalyzeClicked();
    FReply OnExportMarkdownClicked();
    FReply OnExportJsonClicked();

    void PopulateInitialState();

    TArray<TSharedPtr<FRivaUiFinding>> FindingsList;
    TSharedPtr<SListView<TSharedPtr<FRivaUiFinding>>> FindingsListView;
    TSharedPtr<class STextBlock> DetailsTitleText;
    TSharedPtr<class STextBlock> DetailsContentText;
    TSharedPtr<class STextBlock> StatusBarText;
};
