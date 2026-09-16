#pragma once

#include "Modules/ModuleManager.h"

class FEasyThumbnailGeneratorModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    void RegisterMenus();
    void ExtendAssetMenu(const FName MenuName);
};
