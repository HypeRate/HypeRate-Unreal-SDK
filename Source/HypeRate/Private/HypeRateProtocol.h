#pragma once

#include "CoreMinimal.h"

class FHypeRateProtocol
{
public:
    static FString CreateJoin(const FString& DeviceId, int32 Reference);
    static FString CreateLeave(const FString& DeviceId, int32 Reference);
    static FString CreateHeartbeat(int32 Reference);
    static bool TryParseHeartRate(const FString& Json, int32& OutHeartRate);

private:
    static FString CreateChannelMessage(const FString& Topic, const FString& Event, int32 Reference);
};

