#pragma once

#include "CoreMinimal.h"
#include "HypeRateConnectionState.generated.h"

UENUM(BlueprintType)
enum class EHypeRateConnectionState : uint8
{
    Disconnected UMETA(DisplayName = "Disconnected"),
    Connecting UMETA(DisplayName = "Connecting"),
    Connected UMETA(DisplayName = "Connected"),
    Simulating UMETA(DisplayName = "Simulating"),
    Reconnecting UMETA(DisplayName = "Reconnecting"),
    Disconnecting UMETA(DisplayName = "Disconnecting"),
    Error UMETA(DisplayName = "Error")
};

