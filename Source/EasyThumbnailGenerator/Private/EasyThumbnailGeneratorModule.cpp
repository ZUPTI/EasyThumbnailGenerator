#include "EasyThumbnailGeneratorModule.h"

#include "ContentBrowserMenuContexts.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Framework/Application/SlateApplication.h"
#include "SEasyThumbnailGeneratorWindow.h"
#include "ToolMenu.h"
#include "ToolMenus.h"
#include "Widgets/SWindow.h"

#define LOCTEXT_NAMESPACE "EasyThumbnailGenerator"

namespace EasyThumbnailGenerator
{
    void OpenThumbnailWindow(const TArray<FAssetData>& SelectedAssets)
    {
        if (SelectedAssets.IsEmpty())
        {
            return;
        }

        TSharedRef<SWindow> Window = SNew(SWindow)
            .Title(LOCTEXT("ThumbnailWindowTitle", "Easy Thumbnail Generator"))
            .ClientSize(FVector2D(1400.0f, 900.0f))
            .SupportsMaximize(true)
            .SupportsMinimize(false);

        Window->SetContent(
            SNew(SEasyThumbnailGeneratorWindow)
            .SelectedAssets(SelectedAssets));

        FSlateApplication::Get().AddWindow(Window);
    }

    void BuildMeshContextMenu(FToolMenuSection& Section)
    {
        const UContentBrowserAssetContextMenuContext* Context = Section.FindContext<UContentBrowserAssetContextMenuContext>();
        if (!Context || Context->SelectedAssets.IsEmpty())
        {
            return;
        }

        TArray<FAssetData> SupportedAssets;
        SupportedAssets.Reserve(Context->SelectedAssets.Num());

        const FTopLevelAssetPath StaticMeshClassPath = UStaticMesh::StaticClass()->GetClassPathName();
        const FTopLevelAssetPath SkeletalMeshClassPath = USkeletalMesh::StaticClass()->GetClassPathName();

        for (const FAssetData& AssetData : Context->SelectedAssets)
        {
            if (AssetData.AssetClassPath == StaticMeshClassPath || AssetData.AssetClassPath == SkeletalMeshClassPath)
            {
                SupportedAssets.Add(AssetData);
            }
        }

        if (SupportedAssets.IsEmpty())
        {
            return;
        }

        Section.AddMenuEntry(
            TEXT("EasyThumbnailGenerator_CreatePNGThumbnail"),
            LOCTEXT("CreatePNGThumbnail", "Create PNG Thumbnail..."),
            LOCTEXT("CreatePNGThumbnailTooltip", "Open the Easy Thumbnail Generator preview window for the selected mesh assets."),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateLambda([SupportedAssets = MoveTemp(SupportedAssets)]()
            {
                OpenThumbnailWindow(SupportedAssets);
            })),
            EUserInterfaceActionType::Button,
            NAME_None);
    }
}

void FEasyThumbnailGeneratorModule::StartupModule()
{
    if (IsRunningCommandlet())
    {
        return;
    }

    UToolMenus::RegisterStartupCallback(
        FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FEasyThumbnailGeneratorModule::RegisterMenus));
}

void FEasyThumbnailGeneratorModule::ShutdownModule()
{
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);
}

void FEasyThumbnailGeneratorModule::RegisterMenus()
{
    FToolMenuOwnerScoped OwnerScoped(this);

    ExtendAssetMenu(TEXT("ContentBrowser.AssetContextMenu.StaticMesh"));
    ExtendAssetMenu(TEXT("ContentBrowser.AssetContextMenu.SkeletalMesh"));
}

void FEasyThumbnailGeneratorModule::ExtendAssetMenu(const FName MenuName)
{
    UToolMenu* Menu = UToolMenus::Get()->ExtendMenu(MenuName);
    if (!Menu)
    {
        return;
    }

    FToolMenuSection& Section = Menu->FindOrAddSection(TEXT("GetAssetActions"));
    Section.AddDynamicEntry(
        TEXT("EasyThumbnailGenerator_DynamicEntry"),
        FNewToolMenuSectionDelegate::CreateStatic(&EasyThumbnailGenerator::BuildMeshContextMenu));
}

IMPLEMENT_MODULE(FEasyThumbnailGeneratorModule, EasyThumbnailGenerator)

#undef LOCTEXT_NAMESPACE
