#include "SRivaPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "SRivaPanel"

DEFINE_LOG_CATEGORY_STATIC(LogRivaEditor, Log, All);

void SRivaPanel::Construct(const FArguments& InArgs)
{
    UE_LOG(LogRivaEditor, Log, TEXT("Constructing SRivaPanel dockable tab widget."));

    ChildSlot
    [
        SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(FMargin(8.0f))
        [
            SNew(SVerticalBox)

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 0.0f, 0.0f, 8.0f)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("PanelHeader", "Riva Deterministic Performance Diagnostics"))
                .Font(FAppStyle::GetFontStyle("HeadingExtraSmall"))
            ]

            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("WelcomeText", "Riva Performance Companion — Ready for trace analysis."))
                .ColorAndOpacity(FSlateColor::UseSubduedForeground())
            ]
        ]
    ];
}

#undef LOCTEXT_NAMESPACE
