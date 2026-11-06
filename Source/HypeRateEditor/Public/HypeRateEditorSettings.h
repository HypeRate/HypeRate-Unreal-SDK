#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "HypeRateEditorSettings.generated.h"

/** Local, per-user credentials used only while testing in the Unreal Editor. */
UCLASS(Config = EditorPerProjectUserSettings, meta = (DisplayName = "HypeRate"))
class HYPERATEEDITOR_API UHypeRateEditorSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UHypeRateEditorSettings();

    UPROPERTY(Config, EditAnywhere, Category = "Connection", meta = (PasswordField = true))
    FString ApiToken;

    UPROPERTY(Config, EditAnywhere, Category = "Connection", meta = (DisplayName = "Device / Session ID"))
    FString DeviceId;

    UPROPERTY(Config, EditAnywhere, Category = "Connection")
    FString Endpoint;
};

