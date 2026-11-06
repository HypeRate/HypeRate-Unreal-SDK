#include "HypeRateProtocol.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FString FHypeRateProtocol::CreateJoin(const FString& DeviceId, const int32 Reference)
{
    return CreateChannelMessage(FString::Printf(TEXT("hr:%s"), *DeviceId), TEXT("phx_join"), Reference);
}

FString FHypeRateProtocol::CreateLeave(const FString& DeviceId, const int32 Reference)
{
    return CreateChannelMessage(FString::Printf(TEXT("hr:%s"), *DeviceId), TEXT("phx_leave"), Reference);
}

FString FHypeRateProtocol::CreateHeartbeat(const int32 Reference)
{
    return CreateChannelMessage(TEXT("phoenix"), TEXT("heartbeat"), Reference);
}

bool FHypeRateProtocol::TryParseHeartRate(const FString& Json, int32& OutHeartRate)
{
    OutHeartRate = 0;
    if (Json.TrimStartAndEnd().IsEmpty())
    {
        return false;
    }

    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        return false;
    }

    FString Event;
    if (!Root->TryGetStringField(TEXT("event"), Event) ||
        (Event != TEXT("hr_update") && Event != TEXT("hr_feed")))
    {
        return false;
    }

    const TSharedPtr<FJsonObject>* Payload = nullptr;
    if (!Root->TryGetObjectField(TEXT("payload"), Payload) || Payload == nullptr || !Payload->IsValid())
    {
        return false;
    }

    double HeartRate = 0.0;
    if (!(*Payload)->TryGetNumberField(TEXT("hr"), HeartRate) || HeartRate <= 0.0)
    {
        return false;
    }

    OutHeartRate = FMath::RoundToInt(HeartRate);
    return OutHeartRate > 0;
}

FString FHypeRateProtocol::CreateChannelMessage(
    const FString& Topic, const FString& Event, const int32 Reference)
{
    const TSharedRef<FJsonObject> Message = MakeShared<FJsonObject>();
    Message->SetStringField(TEXT("topic"), Topic);
    Message->SetStringField(TEXT("event"), Event);
    Message->SetObjectField(TEXT("payload"), MakeShared<FJsonObject>());
    Message->SetNumberField(TEXT("ref"), Reference);

    FString Json;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Json);
    FJsonSerializer::Serialize(Message, Writer);
    return Json;
}

