#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HypeRateSimulationPreset.h"
#include "HypeRateSimulatorComponent.generated.h"

class UHypeRateClientComponent;

UCLASS(ClassGroup = (HypeRate), meta = (BlueprintSpawnableComponent, DisplayName = "HypeRate Simulator"))
class HYPERATE_API UHypeRateSimulatorComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UHypeRateSimulatorComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HypeRate|Simulation")
    TObjectPtr<UHypeRateClientComponent> Client;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HypeRate|Simulation")
    EHypeRateSimulationPreset Preset = EHypeRateSimulationPreset::Workout;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HypeRate|Simulation", meta = (ClampMin = "30", ClampMax = "240"))
    int32 CustomHeartRate = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HypeRate|Simulation", meta = (ClampMin = "0.1"))
    float UpdateInterval = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HypeRate|Simulation")
    bool bPlayOnBeginPlay = true;

    UFUNCTION(BlueprintCallable, Category = "HypeRate|Simulation")
    void StartSimulation();

    UFUNCTION(BlueprintCallable, Category = "HypeRate|Simulation")
    void StopSimulation();

    UFUNCTION(BlueprintCallable, Category = "HypeRate|Simulation")
    void SetPreset(EHypeRateSimulationPreset NewPreset);

    UFUNCTION(BlueprintCallable, Category = "HypeRate|Simulation")
    void SetCustomHeartRate(int32 HeartRate);

    UFUNCTION(BlueprintPure, Category = "HypeRate|Simulation")
    bool IsRunning() const { return bRunning; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

private:
    bool bRunning = false;
    float SmoothedHeartRate = 0.0f;
    float NoiseOffset = 3.17f;
    double NextUpdateTime = 0.0;

    bool ResolveClient();
    void GetProfile(float& OutCenter, float& OutVariation) const;
    float CalculateTargetHeartRate(double Time) const;
};

