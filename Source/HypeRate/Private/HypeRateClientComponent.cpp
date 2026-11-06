#include "HypeRateClientComponent.h"

#include "Async/Async.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "HAL/PlatformTime.h"
#include "HypeRateProtocol.h"
#include "IWebSocket.h"
#include "Modules/ModuleManager.h"
#include "WebSocketsModule.h"

#if WITH_EDITOR
#include "Misc/ConfigCacheIni.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogHypeRateClient, Log, All);

namespace
{
    void DispatchToGameThread(TFunction<void()>&& Callback)
    {
        if (IsInGameThread())
        {
            Callback();
        }
        else
        {
            AsyncTask(ENamedThreads::GameThread, MoveTemp(Callback));
        }
    }
}

const TCHAR* UHypeRateClientComponent::DefaultEndpoint = TEXT("wss://app.hyperate.io/socket/websocket");

UHypeRateClientComponent::UHypeRateClientComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    RuntimeEndpoint = DefaultEndpoint;
}

void UHypeRateClientComponent::BeginPlay()
{
    Super::BeginPlay();

    if (bConnectOnBeginPlay)
    {
        Connect();
    }
}

void UHypeRateClientComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bEndingPlay = true;
    bIntentionalDisconnect = true;
    bSimulationActive = false;
    bReconnectScheduled = false;
    ReleaseSocket(true);
    Super::EndPlay(EndPlayReason);
}

void UHypeRateClientComponent::TickComponent(
    const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bEndingPlay || bSimulationActive)
    {
        return;
    }

    const double Now = FPlatformTime::Seconds();
    if (Socket.IsValid() && Socket->IsConnected() && Now >= NextHeartbeatTime)
    {
        SendText(FHypeRateProtocol::CreateHeartbeat(NextReference()));
        NextHeartbeatTime = Now + FMath::Max(5.0f, HeartbeatInterval);
    }

    if (bReconnectScheduled && Now >= ReconnectAtTime)
    {
        bReconnectScheduled = false;
        Connect();
    }
}

void UHypeRateClientComponent::Configure(
    const FString& ApiToken, const FString& DeviceId, const FString& Endpoint)
{
    const FString CleanToken = ApiToken.TrimStartAndEnd();
    const FString CleanDeviceId = DeviceId.TrimStartAndEnd();
    const FString CleanEndpoint = Endpoint.TrimStartAndEnd().IsEmpty()
        ? FString(DefaultEndpoint)
        : Endpoint.TrimStartAndEnd();

    if (CleanToken.IsEmpty())
    {
        ReportError(TEXT("A HypeRate API token is required."));
        return;
    }
    if (CleanDeviceId.IsEmpty())
    {
        ReportError(TEXT("A HypeRate Device/Session ID is required."));
        return;
    }
    if (!IsValidEndpoint(CleanEndpoint))
    {
        ReportError(TEXT("The HypeRate endpoint must be a valid ws:// or wss:// URL."));
        return;
    }

    RuntimeApiToken = CleanToken;
    RuntimeDeviceId = CleanDeviceId;
    RuntimeEndpoint = CleanEndpoint;
    LastError.Reset();
}

void UHypeRateClientComponent::ClearConfiguration()
{
    RuntimeApiToken.Reset();
    RuntimeDeviceId.Reset();
    RuntimeEndpoint = DefaultEndpoint;
}

void UHypeRateClientComponent::Connect()
{
    if (bEndingPlay || Socket.IsValid())
    {
        return;
    }

    if (bSimulationActive)
    {
        EndSimulation();
    }

    FString ApiToken;
    FString DeviceId;
    FString Endpoint;
    FString Error;
    if (!TryResolveConfiguration(ApiToken, DeviceId, Endpoint, Error))
    {
        ReportError(Error);
        return;
    }
    if (!IsValidEndpoint(Endpoint))
    {
        ReportError(TEXT("The configured HypeRate endpoint must be a valid ws:// or wss:// URL."));
        return;
    }

    ReleaseSocket(true);
    bReconnectScheduled = false;
    bIntentionalDisconnect = false;
    bDisconnectEventRaised = false;
    LastError.Reset();
    ActiveDeviceId = DeviceId;

    SetState(ReconnectAttempt > 0
        ? EHypeRateConnectionState::Reconnecting
        : EHypeRateConnectionState::Connecting);

    const int32 Generation = ++ConnectionGeneration;
    FWebSocketsModule& WebSockets = FModuleManager::LoadModuleChecked<FWebSocketsModule>(TEXT("WebSockets"));
    const TSharedRef<IWebSocket> NewSocket = WebSockets.CreateWebSocket(BuildConnectionUrl(Endpoint, ApiToken));
    Socket = NewSocket;

    const TWeakObjectPtr<UHypeRateClientComponent> WeakThis(this);
    NewSocket->OnConnected().AddLambda([WeakThis, Generation]()
    {
        DispatchToGameThread([WeakThis, Generation]()
        {
            if (UHypeRateClientComponent* Self = WeakThis.Get())
            {
                Self->HandleConnected(Generation);
            }
        });
    });
    NewSocket->OnMessage().AddLambda([WeakThis, Generation](const FString& Message)
    {
        DispatchToGameThread([WeakThis, Generation, Message]()
        {
            if (UHypeRateClientComponent* Self = WeakThis.Get())
            {
                Self->HandleMessage(Generation, Message);
            }
        });
    });
    NewSocket->OnConnectionError().AddLambda([WeakThis, Generation](const FString& ErrorMessage)
    {
        DispatchToGameThread([WeakThis, Generation, ErrorMessage]()
        {
            if (UHypeRateClientComponent* Self = WeakThis.Get())
            {
                Self->HandleConnectionError(Generation, ErrorMessage);
            }
        });
    });
    NewSocket->OnClosed().AddLambda(
        [WeakThis, Generation](const int32 StatusCode, const FString& Reason, const bool bWasClean)
    {
        DispatchToGameThread([WeakThis, Generation, StatusCode, Reason, bWasClean]()
        {
            if (UHypeRateClientComponent* Self = WeakThis.Get())
            {
                Self->HandleClosed(Generation, StatusCode, Reason, bWasClean);
            }
        });
    });

    NewSocket->Connect();
}

void UHypeRateClientComponent::Disconnect()
{
    bIntentionalDisconnect = true;
    bReconnectScheduled = false;
    ReconnectAttempt = 0;

    if (bSimulationActive)
    {
        EndSimulation();
        return;
    }

    if (!Socket.IsValid())
    {
        SetState(EHypeRateConnectionState::Disconnected);
        RaiseDisconnectedOnce();
        return;
    }

    SetState(EHypeRateConnectionState::Disconnecting);
    if (Socket->IsConnected() && !ActiveDeviceId.IsEmpty())
    {
        SendText(FHypeRateProtocol::CreateLeave(ActiveDeviceId, NextReference()));
    }

    ReleaseSocket(true);
    SetState(EHypeRateConnectionState::Disconnected);
    RaiseDisconnectedOnce();
}

void UHypeRateClientComponent::BeginSimulation(const int32 InitialHeartRate)
{
    if (InitialHeartRate <= 0 || bEndingPlay)
    {
        ReportError(TEXT("The simulated heart rate must be greater than zero."));
        return;
    }

    const bool bWasActive = Socket.IsValid() || bSimulationActive || !bDisconnectEventRaised;
    bIntentionalDisconnect = true;
    bReconnectScheduled = false;
    ReleaseSocket(true);
    if (bWasActive)
    {
        RaiseDisconnectedOnce();
    }

    bSimulationActive = true;
    bIntentionalDisconnect = false;
    bDisconnectEventRaised = false;
    LastError.Reset();
    SetState(EHypeRateConnectionState::Simulating);
    OnConnected.Broadcast();
    PublishHeartRate(InitialHeartRate);
}

void UHypeRateClientComponent::SetSimulatedHeartRate(const int32 HeartRate)
{
    if (!bSimulationActive)
    {
        ReportError(TEXT("Call Begin Simulation before sending simulated heart-rate values."));
        return;
    }
    if (HeartRate <= 0)
    {
        ReportError(TEXT("The simulated heart rate must be greater than zero."));
        return;
    }

    PublishHeartRate(HeartRate);
}

void UHypeRateClientComponent::EndSimulation()
{
    if (!bSimulationActive)
    {
        return;
    }

    bSimulationActive = false;
    CurrentHeartRate = 0;
    SetState(EHypeRateConnectionState::Disconnected);
    RaiseDisconnectedOnce();
}

bool UHypeRateClientComponent::IsConnected() const
{
    return State == EHypeRateConnectionState::Connected || bSimulationActive;
}

bool UHypeRateClientComponent::HasConfiguration() const
{
    FString ApiToken;
    FString DeviceId;
    FString Endpoint;
    FString Error;
    return TryResolveConfiguration(ApiToken, DeviceId, Endpoint, Error);
}

void UHypeRateClientComponent::HandleConnected(const int32 Generation)
{
    if (Generation != ConnectionGeneration || !Socket.IsValid() || bEndingPlay)
    {
        return;
    }

    ReconnectAttempt = 0;
    LastError.Reset();
    SetState(EHypeRateConnectionState::Connected);
    NextHeartbeatTime = FPlatformTime::Seconds() + FMath::Max(5.0f, HeartbeatInterval);
    OnConnected.Broadcast();
    SendText(FHypeRateProtocol::CreateJoin(ActiveDeviceId, NextReference()));
}

void UHypeRateClientComponent::HandleMessage(const int32 Generation, const FString& Message)
{
    if (Generation != ConnectionGeneration || bEndingPlay)
    {
        return;
    }

    int32 HeartRate = 0;
    if (FHypeRateProtocol::TryParseHeartRate(Message, HeartRate))
    {
        PublishHeartRate(HeartRate);
    }
}

void UHypeRateClientComponent::HandleConnectionError(const int32 Generation, const FString& ErrorMessage)
{
    if (Generation != ConnectionGeneration || bIntentionalDisconnect || bEndingPlay)
    {
        return;
    }

    const FString Reason = ErrorMessage.IsEmpty() ? TEXT("Unknown WebSocket error.") : ErrorMessage;
    ReportError(Reason);
    HandleConnectionLost(Generation, Reason);
}

void UHypeRateClientComponent::HandleClosed(
    const int32 Generation, const int32 StatusCode, const FString& Reason, const bool bWasClean)
{
    if (Generation != ConnectionGeneration || bEndingPlay)
    {
        return;
    }

    const FString Description = FString::Printf(
        TEXT("Connection closed (%d%s%s)."),
        StatusCode,
        Reason.IsEmpty() ? TEXT("") : TEXT(": "),
        Reason.IsEmpty() ? TEXT("") : *Reason);
    HandleConnectionLost(Generation, bWasClean ? Description : Description + TEXT(" The close was not clean."));
}

void UHypeRateClientComponent::HandleConnectionLost(const int32 Generation, const FString& Reason)
{
    if (Generation != ConnectionGeneration)
    {
        return;
    }

    ReleaseSocket(false);
    SetState(EHypeRateConnectionState::Disconnected);
    RaiseDisconnectedOnce();

    if (!bIntentionalDisconnect && !bEndingPlay && bAutoReconnect)
    {
        ScheduleReconnect(Reason);
    }
}

void UHypeRateClientComponent::ScheduleReconnect(const FString& Reason)
{
    if (bReconnectScheduled)
    {
        return;
    }

    ++ReconnectAttempt;
    const double Delay = FMath::Min(
        FMath::Pow(2.0, static_cast<double>(ReconnectAttempt - 1)),
        static_cast<double>(FMath::Max(1.0f, MaximumReconnectDelay)));
    ReconnectAtTime = FPlatformTime::Seconds() + Delay;
    bReconnectScheduled = true;
    LastError = FString::Printf(TEXT("%s Reconnecting in %.1f seconds."), *Reason, Delay);
    SetState(EHypeRateConnectionState::Reconnecting);
}

void UHypeRateClientComponent::ReleaseSocket(const bool bCloseSocket)
{
    const TSharedPtr<IWebSocket> PreviousSocket = Socket;
    Socket.Reset();
    ++ConnectionGeneration;

    if (bCloseSocket && PreviousSocket.IsValid())
    {
        PreviousSocket->Close(1000, TEXT("HypeRate client disconnect"));
    }
}

void UHypeRateClientComponent::SendText(const FString& Message)
{
    if (Socket.IsValid() && Socket->IsConnected() && !Message.IsEmpty())
    {
        Socket->Send(Message);
    }
}

void UHypeRateClientComponent::PublishHeartRate(const int32 HeartRate)
{
    CurrentHeartRate = HeartRate;
    OnHeartRateChanged.Broadcast(HeartRate);
}

void UHypeRateClientComponent::ReportError(const FString& Message)
{
    if (bEndingPlay)
    {
        return;
    }

    LastError = Message;
    SetState(EHypeRateConnectionState::Error);
    UE_LOG(LogHypeRateClient, Error, TEXT("%s"), *Message);
    OnError.Broadcast(Message);
}

void UHypeRateClientComponent::SetState(const EHypeRateConnectionState NewState)
{
    if (State == NewState)
    {
        return;
    }

    State = NewState;
    OnStateChanged.Broadcast(NewState);
}

void UHypeRateClientComponent::RaiseDisconnectedOnce()
{
    if (bDisconnectEventRaised)
    {
        return;
    }

    bDisconnectEventRaised = true;
    OnDisconnected.Broadcast();
}

int32 UHypeRateClientComponent::NextReference()
{
    if (MessageReference == MAX_int32)
    {
        MessageReference = 0;
    }
    return ++MessageReference;
}

bool UHypeRateClientComponent::TryResolveConfiguration(
    FString& OutApiToken, FString& OutDeviceId, FString& OutEndpoint, FString& OutError) const
{
    if (!RuntimeApiToken.IsEmpty() && !RuntimeDeviceId.IsEmpty())
    {
        OutApiToken = RuntimeApiToken;
        OutDeviceId = RuntimeDeviceId;
        OutEndpoint = RuntimeEndpoint.IsEmpty() ? FString(DefaultEndpoint) : RuntimeEndpoint;
        if (IsValidEndpoint(OutEndpoint))
        {
            OutError.Reset();
            return true;
        }
        OutError = TEXT("The configured HypeRate endpoint must be a valid ws:// or wss:// URL.");
        return false;
    }

#if WITH_EDITOR
    const TCHAR* Section = TEXT("/Script/HypeRateEditor.HypeRateEditorSettings");
    FString EditorToken;
    FString EditorDeviceId;
    FString EditorEndpoint;
    if (GConfig != nullptr)
    {
        GConfig->GetString(Section, TEXT("ApiToken"), EditorToken, GEditorPerProjectIni);
        GConfig->GetString(Section, TEXT("DeviceId"), EditorDeviceId, GEditorPerProjectIni);
        GConfig->GetString(Section, TEXT("Endpoint"), EditorEndpoint, GEditorPerProjectIni);
    }
    EditorToken = EditorToken.TrimStartAndEnd();
    EditorDeviceId = EditorDeviceId.TrimStartAndEnd();
    EditorEndpoint = EditorEndpoint.TrimStartAndEnd();
    if (!EditorToken.IsEmpty() && !EditorDeviceId.IsEmpty())
    {
        OutApiToken = EditorToken;
        OutDeviceId = EditorDeviceId;
        OutEndpoint = EditorEndpoint.IsEmpty() ? FString(DefaultEndpoint) : EditorEndpoint;
        if (IsValidEndpoint(OutEndpoint))
        {
            OutError.Reset();
            return true;
        }
        OutError = TEXT("The configured HypeRate endpoint must be a valid ws:// or wss:// URL.");
        return false;
    }
    OutError = TEXT("HypeRate is not configured for Editor testing. Open Tools > HypeRate Setup.");
#else
    OutError = TEXT("HypeRate has no runtime configuration. Call Configure before connecting.");
#endif

    OutApiToken.Reset();
    OutDeviceId.Reset();
    OutEndpoint = DefaultEndpoint;
    return false;
}

bool UHypeRateClientComponent::IsValidEndpoint(const FString& Endpoint)
{
    const FString CleanEndpoint = Endpoint.TrimStartAndEnd();
    const bool bValidScheme = CleanEndpoint.StartsWith(TEXT("ws://"), ESearchCase::IgnoreCase) ||
        CleanEndpoint.StartsWith(TEXT("wss://"), ESearchCase::IgnoreCase);
    const int32 SchemeEnd = CleanEndpoint.Find(TEXT("://"));
    return bValidScheme && SchemeEnd != INDEX_NONE && CleanEndpoint.Len() > SchemeEnd + 3 &&
        !CleanEndpoint.Contains(TEXT(" "));
}

FString UHypeRateClientComponent::BuildConnectionUrl(const FString& Endpoint, const FString& ApiToken)
{
    FString CleanEndpoint = Endpoint.TrimStartAndEnd();
    while (CleanEndpoint.EndsWith(TEXT("/")))
    {
        CleanEndpoint.LeftChopInline(1);
    }
    const TCHAR* Separator = CleanEndpoint.Contains(TEXT("?")) ? TEXT("&") : TEXT("?");
    return FString::Printf(
        TEXT("%s%stoken=%s"),
        *CleanEndpoint,
        Separator,
        *FGenericPlatformHttp::UrlEncode(ApiToken.TrimStartAndEnd()));
}

