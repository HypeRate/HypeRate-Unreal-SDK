#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogHypeRate, Log, All);

class FHypeRateModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        FModuleManager::Get().LoadModule(TEXT("WebSockets"));
        UE_LOG(LogHypeRate, Log, TEXT("HypeRate runtime module loaded."));
    }
};

IMPLEMENT_MODULE(FHypeRateModule, HypeRate)

