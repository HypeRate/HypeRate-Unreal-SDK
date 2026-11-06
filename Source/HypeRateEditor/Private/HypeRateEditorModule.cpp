#include "HypeRateEditorSettings.h"

#include "ISettingsModule.h"
#include "Modules/ModuleManager.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "FHypeRateEditorModule"

class FHypeRateEditorModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        if (ISettingsModule* Settings = FModuleManager::GetModulePtr<ISettingsModule>(TEXT("Settings")))
        {
            Settings->RegisterSettings(
                TEXT("Project"),
                TEXT("Plugins"),
                TEXT("HypeRate"),
                LOCTEXT("SettingsName", "HypeRate"),
                LOCTEXT("SettingsDescription", "Local HypeRate credentials used for Editor testing only."),
                GetMutableDefault<UHypeRateEditorSettings>());
        }

        UToolMenus::RegisterStartupCallback(
            FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FHypeRateEditorModule::RegisterMenus));
    }

    virtual void ShutdownModule() override
    {
        UToolMenus::UnRegisterStartupCallback(this);
        UToolMenus::UnregisterOwner(this);

        if (ISettingsModule* Settings = FModuleManager::GetModulePtr<ISettingsModule>(TEXT("Settings")))
        {
            Settings->UnregisterSettings(TEXT("Project"), TEXT("Plugins"), TEXT("HypeRate"));
        }
    }

private:
    void RegisterMenus()
    {
        FToolMenuOwnerScoped OwnerScoped(this);
        UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Tools"));
        FToolMenuSection& Section = ToolsMenu->FindOrAddSection(TEXT("Programming"));
        Section.AddMenuEntry(
            TEXT("HypeRateSetup"),
            LOCTEXT("HypeRateSetupLabel", "HypeRate Setup"),
            LOCTEXT("HypeRateSetupTooltip", "Configure local HypeRate credentials for Play In Editor."),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateRaw(this, &FHypeRateEditorModule::OpenSettings)));
    }

    void OpenSettings()
    {
        ISettingsModule& Settings = FModuleManager::LoadModuleChecked<ISettingsModule>(TEXT("Settings"));
        Settings.ShowViewer(TEXT("Project"), TEXT("Plugins"), TEXT("HypeRate"));
    }
};

IMPLEMENT_MODULE(FHypeRateEditorModule, HypeRateEditor)

#undef LOCTEXT_NAMESPACE

