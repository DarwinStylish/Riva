#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class SRivaPanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SRivaPanel) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
};
