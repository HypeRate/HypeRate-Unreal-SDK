#include "HypeRateHeartRateWidget.h"

#include "Components/Widget.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Pawn.h"
#include "HypeRateClientComponent.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

class SHypeRateGraph final : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SHypeRateGraph) {}
        SLATE_ARGUMENT(TWeakObjectPtr<UHypeRateHeartRateWidget>, Owner)
    SLATE_END_ARGS()

    void Construct(const FArguments& Arguments)
    {
        Owner = Arguments._Owner;
    }

    virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override
    {
        return FVector2D(520.0f, 120.0f);
    }

    virtual int32 OnPaint(
        const FPaintArgs& Args,
        const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements,
        int32 LayerId,
        const FWidgetStyle& InWidgetStyle,
        bool bParentEnabled) const override
    {
        const UHypeRateHeartRateWidget* Widget = Owner.Get();
        if (Widget == nullptr || Widget->Samples.Num() < 2)
        {
            return LayerId;
        }

        int32 Minimum = MAX_int32;
        int32 Maximum = MIN_int32;
        for (const int32 Sample : Widget->Samples)
        {
            Minimum = FMath::Min(Minimum, Sample);
            Maximum = FMath::Max(Maximum, Sample);
        }

        const FVector2D Size = AllottedGeometry.GetLocalSize();
        const float Range = FMath::Max(1.0f, static_cast<float>(Maximum - Minimum));
        TArray<FVector2D> Points;
        Points.Reserve(Widget->Samples.Num());
        for (int32 Index = 0; Index < Widget->Samples.Num(); ++Index)
        {
            const float X = Size.X * Index / FMath::Max(1, Widget->Samples.Num() - 1);
            const float Normalized = (Widget->Samples[Index] - Minimum) / Range;
            const float Y = Size.Y - (Normalized * (Size.Y - 12.0f) + 6.0f);
            Points.Emplace(X, Y);
        }

        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(),
            Points,
            ESlateDrawEffect::None,
            FLinearColor(0.06f, 0.45f, 0.96f, 1.0f),
            true,
            3.0f);
        return LayerId + 1;
    }

private:
    TWeakObjectPtr<UHypeRateHeartRateWidget> Owner;
};

class SHypeRateCard final : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SHypeRateCard) {}
        SLATE_ARGUMENT(TWeakObjectPtr<UHypeRateHeartRateWidget>, Owner)
    SLATE_END_ARGS()

    void Construct(const FArguments& Arguments)
    {
        Owner = Arguments._Owner;

        ChildSlot
        [
            SNew(SBorder)
            .BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
            .BorderBackgroundColor(FLinearColor(0.035f, 0.038f, 0.048f, 0.98f))
            .Padding(FMargin(32.0f, 24.0f))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    .VAlign(VAlign_Center)
                    [
                        SNew(STextBlock)
                        .Text_Lambda([this]()
                        {
                            const UHypeRateHeartRateWidget* Widget = Owner.Get();
                            return Widget != nullptr ? Widget->GetHeartRateText() : FText::FromString(TEXT("--"));
                        })
                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 72))
                        .ColorAndOpacity(FLinearColor::White)
                    ]
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Bottom)
                    .Padding(8.0f, 0.0f, 24.0f, 14.0f)
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("BPM")))
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 22))
                        .ColorAndOpacity(FLinearColor(0.64f, 0.65f, 0.72f, 1.0f))
                    ]
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    [
                        SNew(STextBlock)
                        .Text_Lambda([this]()
                        {
                            const UHypeRateHeartRateWidget* Widget = Owner.Get();
                            return Widget != nullptr ? Widget->GetStatusText() : FText::FromString(TEXT("OFFLINE"));
                        })
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
                        .ColorAndOpacity_Lambda([this]()
                        {
                            const UHypeRateHeartRateWidget* Widget = Owner.Get();
                            return Widget != nullptr ? Widget->GetStatusColor() : FLinearColor::Red;
                        })
                    ]
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 16.0f, 0.0f, 0.0f)
                [
                    SNew(SBox)
                    .HeightOverride(120.0f)
                    [
                        SNew(SHypeRateGraph)
                        .Owner(Owner)
                    ]
                ]
            ]
        ];
    }

private:
    TWeakObjectPtr<UHypeRateHeartRateWidget> Owner;
};

void UHypeRateHeartRateWidget::SetClient(UHypeRateClientComponent* NewClient)
{
    if (Client == NewClient)
    {
        return;
    }

    UnbindClient();
    Client = NewClient;
    BindClient();
    InvalidateCard();
}

void UHypeRateHeartRateWidget::PushHeartRate(const int32 HeartRate)
{
    if (HeartRate <= 0)
    {
        return;
    }

    CurrentHeartRate = HeartRate;
    Samples.Add(HeartRate);
    const int32 Limit = FMath::Clamp(MaximumSamples, 5, 120);
    if (Samples.Num() > Limit)
    {
        Samples.RemoveAt(0, Samples.Num() - Limit, EAllowShrinking::No);
    }
    InvalidateCard();
}

TSharedRef<SWidget> UHypeRateHeartRateWidget::RebuildWidget()
{
    return SAssignNew(CardWidget, SHypeRateCard).Owner(TWeakObjectPtr<UHypeRateHeartRateWidget>(this));
}

void UHypeRateHeartRateWidget::NativeConstruct()
{
    Super::NativeConstruct();
    ResolveClient();
    BindClient();
    if (Client != nullptr && Client->CurrentHeartRate > 0)
    {
        PushHeartRate(Client->CurrentHeartRate);
    }
}

void UHypeRateHeartRateWidget::NativeDestruct()
{
    UnbindClient();
    Super::NativeDestruct();
}

void UHypeRateHeartRateWidget::ReleaseSlateResources(const bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);
    CardWidget.Reset();
}

void UHypeRateHeartRateWidget::HandleHeartRateChanged(const int32 HeartRate)
{
    PushHeartRate(HeartRate);
}

void UHypeRateHeartRateWidget::HandleStateChanged(const EHypeRateConnectionState NewState)
{
    InvalidateCard();
}

void UHypeRateHeartRateWidget::ResolveClient()
{
    if (Client != nullptr)
    {
        return;
    }

    if (APawn* Pawn = GetOwningPlayerPawn())
    {
        Client = Pawn->FindComponentByClass<UHypeRateClientComponent>();
    }
}

void UHypeRateHeartRateWidget::BindClient()
{
    if (Client == nullptr)
    {
        return;
    }

    Client->OnHeartRateChanged.RemoveDynamic(this, &UHypeRateHeartRateWidget::HandleHeartRateChanged);
    Client->OnStateChanged.RemoveDynamic(this, &UHypeRateHeartRateWidget::HandleStateChanged);
    Client->OnHeartRateChanged.AddDynamic(this, &UHypeRateHeartRateWidget::HandleHeartRateChanged);
    Client->OnStateChanged.AddDynamic(this, &UHypeRateHeartRateWidget::HandleStateChanged);
}

void UHypeRateHeartRateWidget::UnbindClient()
{
    if (Client == nullptr)
    {
        return;
    }

    Client->OnHeartRateChanged.RemoveDynamic(this, &UHypeRateHeartRateWidget::HandleHeartRateChanged);
    Client->OnStateChanged.RemoveDynamic(this, &UHypeRateHeartRateWidget::HandleStateChanged);
}

FText UHypeRateHeartRateWidget::GetHeartRateText() const
{
    return CurrentHeartRate > 0 ? FText::AsNumber(CurrentHeartRate) : FText::FromString(TEXT("--"));
}

FText UHypeRateHeartRateWidget::GetStatusText() const
{
    const EHypeRateConnectionState CurrentState = Client != nullptr
        ? Client->State
        : EHypeRateConnectionState::Disconnected;

    switch (CurrentState)
    {
    case EHypeRateConnectionState::Connected:
        return FText::FromString(TEXT("LIVE"));
    case EHypeRateConnectionState::Connecting:
        return FText::FromString(TEXT("CONNECTING"));
    case EHypeRateConnectionState::Reconnecting:
        return FText::FromString(TEXT("RECONNECTING"));
    case EHypeRateConnectionState::Disconnecting:
        return FText::FromString(TEXT("DISCONNECTING"));
    case EHypeRateConnectionState::Simulating:
        return FText::FromString(TEXT("SIMULATION"));
    case EHypeRateConnectionState::Error:
        return FText::FromString(TEXT("ERROR"));
    default:
        return FText::FromString(TEXT("OFFLINE"));
    }
}

FLinearColor UHypeRateHeartRateWidget::GetStatusColor() const
{
    const EHypeRateConnectionState CurrentState = Client != nullptr
        ? Client->State
        : EHypeRateConnectionState::Disconnected;

    switch (CurrentState)
    {
    case EHypeRateConnectionState::Connected:
        return FLinearColor(0.16f, 0.86f, 0.43f, 1.0f);
    case EHypeRateConnectionState::Simulating:
        return FLinearColor(0.21f, 0.62f, 1.0f, 1.0f);
    case EHypeRateConnectionState::Connecting:
    case EHypeRateConnectionState::Reconnecting:
    case EHypeRateConnectionState::Disconnecting:
        return FLinearColor(1.0f, 0.72f, 0.16f, 1.0f);
    default:
        return FLinearColor(1.0f, 0.22f, 0.24f, 1.0f);
    }
}

void UHypeRateHeartRateWidget::InvalidateCard()
{
    if (CardWidget.IsValid())
    {
        CardWidget->Invalidate(EInvalidateWidgetReason::Paint | EInvalidateWidgetReason::Layout);
    }
}

