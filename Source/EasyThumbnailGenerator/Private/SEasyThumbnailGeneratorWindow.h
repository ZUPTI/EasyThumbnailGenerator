#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"
#include "EditorViewportClient.h"
#include "SEditorViewport.h"
#include "Widgets/SCompoundWidget.h"
#include "EasyThumbnailGeneratorSessionSettings.h"

class FPreviewScene;
class IDetailsView;
class UPrimitiveComponent;

class FEasyThumbnailGeneratorViewportClient : public FEditorViewportClient
{
public:
    FEasyThumbnailGeneratorViewportClient(FPreviewScene& InPreviewScene, const TSharedRef<SEditorViewport>& InViewportWidget);

    void Tick(float DeltaSeconds) override;

    void SetAsset(UObject* InAsset);
    void ApplySettings(const FEasyThumbnailGeneratorRenderSettings& InSettings, bool bRefitCamera);
    void FitCameraToAsset();
    FEasyThumbnailGeneratorRenderSettings GetCurrentRenderSettings() const;

private:
    bool GetAssetBounds(FBoxSphereBounds& OutBounds) const;
    UPrimitiveComponent* BuildPreviewComponent(UObject* Asset) const;
    void ClearPreviewComponent();
    void RefreshScene();
    void SyncViewToSettings(bool bRefitCamera);

private:
    FPreviewScene& PreviewScene;
    TObjectPtr<UPrimitiveComponent> PreviewComponent;
    TWeakObjectPtr<UObject> Asset;
    FEasyThumbnailGeneratorRenderSettings CurrentSettings;
};

class SEasyThumbnailGeneratorViewport : public SEditorViewport
{
public:
    SLATE_BEGIN_ARGS(SEasyThumbnailGeneratorViewport) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    void SetAsset(UObject* InAsset);
    void ApplySettings(const FEasyThumbnailGeneratorRenderSettings& InSettings, bool bRefitCamera);
    void FitCameraToAsset();
    FEasyThumbnailGeneratorRenderSettings GetCurrentRenderSettings() const;

protected:
    TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
    void BindCommands() override;

private:
    TUniquePtr<FPreviewScene> PreviewScene;
    TSharedPtr<FEasyThumbnailGeneratorViewportClient> ViewportClient;
};

class SEasyThumbnailGeneratorWindow : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SEasyThumbnailGeneratorWindow) {}
        SLATE_ARGUMENT(TArray<FAssetData>, SelectedAssets)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    void HandleSettingsChanged(const FPropertyChangedEvent& PropertyChangedEvent);
    FReply HandleGenerateClicked();
    FReply HandleFitClicked();
    FReply HandleResetClicked();
    void ApplyPresetIfNeeded(FName ChangedPropertyName);
    void RefreshViewport(bool bRefitCamera);
    FEasyThumbnailGeneratorRenderSettings BuildRenderSettingsFromViewport() const;
    FText GetSelectionLabel() const;

private:
    TArray<FAssetData> SelectedAssets;
    TStrongObjectPtr<UEasyThumbnailGeneratorSessionSettings> SessionSettings;
    TSharedPtr<SEasyThumbnailGeneratorViewport> ViewportWidget;
    TSharedPtr<IDetailsView> DetailsView;
    bool bApplyingPreset = false;
};
