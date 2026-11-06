#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HypeRateConnectionState.h"
#include "HypeRateHeartRateWidget.generated.h"

class SHypeRateCard;
class SHypeRateGraph;
class UHypeRateClientComponent;

/** Native UMG widget with BPM, status, and a compact history graph. */
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "HypeRate Heart Rate Widget"))
class HYPERATE_API UHypeRateHeartRateWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HypeRate")
    TObjectPtr<UHypeRateClientComponent> Client;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HypeRate", meta = (ClampMin = "5", ClampMax = "120"))
    int32 MaximumSamples = 30;

    UFUNCTION(BlueprintCallable, Category = "HypeRate")
    void SetClient(UHypeRateClientComponent* NewClient);

    UFUNCTION(BlueprintCallable, Category = "HypeRate")
    void PushHeartRate(int32 HeartRate);

    UFUNCTION(BlueprintPure, Category = "HypeRate")
    int32 GetCurrentHeartRate() const { return CurrentHeartRate; }

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
    friend class SHypeRateCard;
    friend class SHypeRateGraph;

    TArray<int32> Samples;
    TSharedPtr<SHypeRateCard> CardWidget;
    int32 CurrentHeartRate = 0;

    UFUNCTION()
    void HandleHeartRateChanged(int32 HeartRate);

    UFUNCTION()
    void HandleStateChanged(EHypeRateConnectionState NewState);

    void ResolveClient();
    void BindClient();
    void UnbindClient();
    FText GetHeartRateText() const;
    FText GetStatusText() const;
    FLinearColor GetStatusColor() const;
    void InvalidateCard();
};

