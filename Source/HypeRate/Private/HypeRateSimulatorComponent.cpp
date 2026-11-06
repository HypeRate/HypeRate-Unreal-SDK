#include "HypeRateSimulatorComponent.h"

#include "HAL/PlatformTime.h"
#include "HypeRateClientComponent.h"
#include "GameFramework/Actor.h"

UHypeRateSimulatorComponent::UHypeRateSimulatorComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UHypeRateSimulatorComponent::BeginPlay()
{
    Super::BeginPlay();
    NoiseOffset += GetUniqueID() * 0.000137f;

    if (bPlayOnBeginPlay)
    {
        StartSimulation();
    }
}

void UHypeRateSimulatorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopSimulation();
    Super::EndPlay(EndPlayReason);
}

void UHypeRateSimulatorComponent::TickComponent(
    const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    const double Now = FPlatformTime::Seconds();
    if (!bRunning || Client == nullptr || Now < NextUpdateTime)
    {
        return;
    }

    NextUpdateTime = Now + FMath::Max(0.1f, UpdateInterval);
    const float Target = CalculateTargetHeartRate(Now);
    SmoothedHeartRate = FMath::FInterpConstantTo(
        SmoothedHeartRate, Target, FMath::Max(0.1f, UpdateInterval), 20.0f);
    Client->SetSimulatedHeartRate(FMath::Clamp(FMath::RoundToInt(SmoothedHeartRate), 30, 240));
}

void UHypeRateSimulatorComponent::StartSimulation()
{
    if (!ResolveClient())
    {
        UE_LOG(LogTemp, Error, TEXT("[HypeRate] The simulator needs a HypeRate Client component."));
        return;
    }

    float Center = 0.0f;
    float Variation = 0.0f;
    GetProfile(Center, Variation);
    SmoothedHeartRate = Center;
    bRunning = true;
    NextUpdateTime = 0.0;
    Client->BeginSimulation(FMath::RoundToInt(Center));
}

void UHypeRateSimulatorComponent::StopSimulation()
{
    if (!bRunning)
    {
        return;
    }

    bRunning = false;
    if (Client != nullptr)
    {
        Client->EndSimulation();
    }
}

void UHypeRateSimulatorComponent::SetPreset(const EHypeRateSimulationPreset NewPreset)
{
    Preset = NewPreset;
    float Center = 0.0f;
    float Variation = 0.0f;
    GetProfile(Center, Variation);
    SmoothedHeartRate = Center;

    if (bRunning && Client != nullptr)
    {
        Client->SetSimulatedHeartRate(FMath::RoundToInt(Center));
    }
}

void UHypeRateSimulatorComponent::SetCustomHeartRate(const int32 HeartRate)
{
    CustomHeartRate = FMath::Clamp(HeartRate, 30, 240);
    SetPreset(EHypeRateSimulationPreset::Custom);
}

bool UHypeRateSimulatorComponent::ResolveClient()
{
    if (Client == nullptr && GetOwner() != nullptr)
    {
        Client = GetOwner()->FindComponentByClass<UHypeRateClientComponent>();
    }
    return Client != nullptr;
}

void UHypeRateSimulatorComponent::GetProfile(float& OutCenter, float& OutVariation) const
{
    switch (Preset)
    {
    case EHypeRateSimulationPreset::Resting:
        OutCenter = 64.0f;
        OutVariation = 4.0f;
        break;
    case EHypeRateSimulationPreset::Intense:
        OutCenter = 168.0f;
        OutVariation = 12.0f;
        break;
    case EHypeRateSimulationPreset::Custom:
        OutCenter = static_cast<float>(CustomHeartRate);
        OutVariation = 2.0f;
        break;
    default:
        OutCenter = 126.0f;
        OutVariation = 9.0f;
        break;
    }
}

float UHypeRateSimulatorComponent::CalculateTargetHeartRate(const double Time) const
{
    float Center = 0.0f;
    float Variation = 0.0f;
    GetProfile(Center, Variation);
    const float TimeValue = static_cast<float>(Time);
    const float SlowWave = FMath::Sin((TimeValue + NoiseOffset) * 0.62f) * Variation * 0.35f;
    const float Noise = FMath::PerlinNoise1D(TimeValue * 0.11f + NoiseOffset) * Variation;
    return FMath::Clamp(Center + SlowWave + Noise, 30.0f, 240.0f);
}

