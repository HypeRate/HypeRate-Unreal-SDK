#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HypeRateConnectionState.h"
#include "HypeRateClientComponent.generated.h"

class IWebSocket;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHypeRateHeartRateChanged, int32, HeartRate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHypeRateConnectionEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHypeRateErrorEvent, const FString&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHypeRateStateChanged, EHypeRateConnectionState, NewState);

/**
 * Blueprint-friendly HypeRate client. Add it to any Actor, configure it at runtime,
 * and bind to OnHeartRateChanged. All public events are delivered on the game thread.
 */
UCLASS(ClassGroup = (HypeRate), meta = (BlueprintSpawnableComponent, DisplayName = "HypeRate Client"))
class HYPERATE_API UHypeRateClientComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UHypeRateClientComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HypeRate|Connection")
    bool bConnectOnBeginPlay = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HypeRate|Connection")
    bool bAutoReconnect = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HypeRate|Connection", meta = (ClampMin = "5.0", UIMin = "5.0"))
    float HeartbeatInterval = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HypeRate|Connection", meta = (ClampMin = "1.0", UIMin = "1.0"))
    float MaximumReconnectDelay = 30.0f;

    UPROPERTY(BlueprintAssignable, Category = "HypeRate|Events")
    FHypeRateHeartRateChanged OnHeartRateChanged;

    UPROPERTY(BlueprintAssignable, Category = "HypeRate|Events")
    FHypeRateConnectionEvent OnConnected;

    UPROPERTY(BlueprintAssignable, Category = "HypeRate|Events")
    FHypeRateConnectionEvent OnDisconnected;

    UPROPERTY(BlueprintAssignable, Category = "HypeRate|Events")
    FHypeRateErrorEvent OnError;

    UPROPERTY(BlueprintAssignable, Category = "HypeRate|Events")
    FHypeRateStateChanged OnStateChanged;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HypeRate|Status")
    EHypeRateConnectionState State = EHypeRateConnectionState::Disconnected;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HypeRate|Status")
    int32 CurrentHeartRate = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HypeRate|Status")
    FString LastError;

    UFUNCTION(BlueprintCallable, Category = "HypeRate")
    void Configure(const FString& ApiToken, const FString& DeviceId,
        const FString& Endpoint = TEXT("wss://app.hyperate.io/socket/websocket"));

    UFUNCTION(BlueprintCallable, Category = "HypeRate")
    void ClearConfiguration();

    UFUNCTION(BlueprintCallable, Category = "HypeRate")
    void Connect();

    UFUNCTION(BlueprintCallable, Category = "HypeRate")
    void Disconnect();

    UFUNCTION(BlueprintCallable, Category = "HypeRate|Simulation")
    void BeginSimulation(int32 InitialHeartRate = 72);

    UFUNCTION(BlueprintCallable, Category = "HypeRate|Simulation")
    void SetSimulatedHeartRate(int32 HeartRate);

    UFUNCTION(BlueprintCallable, Category = "HypeRate|Simulation")
    void EndSimulation();

    UFUNCTION(BlueprintPure, Category = "HypeRate|Status")
    bool IsConnected() const;

    UFUNCTION(BlueprintPure, Category = "HypeRate|Status")
    bool IsSimulating() const { return bSimulationActive; }

    UFUNCTION(BlueprintPure, Category = "HypeRate|Status")
    bool HasConfiguration() const;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

private:
    static const TCHAR* DefaultEndpoint;

    TSharedPtr<IWebSocket> Socket;
    FString RuntimeApiToken;
    FString RuntimeDeviceId;
    FString RuntimeEndpoint;
    FString ActiveDeviceId;
    int32 ConnectionGeneration = 0;
    int32 ReconnectAttempt = 0;
    int32 MessageReference = 0;
    double NextHeartbeatTime = 0.0;
    double ReconnectAtTime = 0.0;
    bool bReconnectScheduled = false;
    bool bIntentionalDisconnect = false;
    bool bSimulationActive = false;
    bool bDisconnectEventRaised = true;
    bool bEndingPlay = false;

    void HandleConnected(int32 Generation);
    void HandleMessage(int32 Generation, const FString& Message);
    void HandleConnectionError(int32 Generation, const FString& ErrorMessage);
    void HandleClosed(int32 Generation, int32 StatusCode, const FString& Reason, bool bWasClean);
    void HandleConnectionLost(int32 Generation, const FString& Reason);
    void ScheduleReconnect(const FString& Reason);
    void ReleaseSocket(bool bCloseSocket);
    void SendText(const FString& Message);
    void PublishHeartRate(int32 HeartRate);
    void ReportError(const FString& Message);
    void SetState(EHypeRateConnectionState NewState);
    void RaiseDisconnectedOnce();
    int32 NextReference();
    bool TryResolveConfiguration(FString& OutApiToken, FString& OutDeviceId,
        FString& OutEndpoint, FString& OutError) const;
    static bool IsValidEndpoint(const FString& Endpoint);
    static FString BuildConnectionUrl(const FString& Endpoint, const FString& ApiToken);
};

