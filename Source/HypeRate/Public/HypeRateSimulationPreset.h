#pragma once

#include "CoreMinimal.h"
#include "HypeRateSimulationPreset.generated.h"

UENUM(BlueprintType)
enum class EHypeRateSimulationPreset : uint8
{
    Resting UMETA(DisplayName = "Resting"),
    Workout UMETA(DisplayName = "Workout"),
    Intense UMETA(DisplayName = "Intense"),
    Custom UMETA(DisplayName = "Custom")
};

