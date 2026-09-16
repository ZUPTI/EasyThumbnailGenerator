#pragma once

#include "AssetRegistry/AssetData.h"
#include "CoreMinimal.h"
#include "EasyThumbnailGeneratorSessionSettings.h"
#include "EditorViewportClient.h"
#include "SEditorViewport.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SComboBox.h"
#include "UObject/UnrealType.h"

class FPreviewScene;
class IDetailsView;
class SEditableTextBox;
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
    UPrimitiveComponent* BuildPreviewComponent(UObject* InAsset) const;
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
    virtual ~SEasyThumbnailGeneratorWindow() override;

private:
    void HandleSettingsChanged(const FPropertyChangedEvent& PropertyChangedEvent);

    FReply HandleGenerateCurrentClicked();
    FReply HandleGenerateAllClicked();
    FReply HandleFitClicked();
    FReply HandleResetClicked();
    FReply HandlePreviousAssetClicked();
    FReply HandleNextAssetClicked();
    FReply HandleCameraShortcut(float Yaw, float Pitch);

    FReply HandleSavePresetClicked();
    FReply HandleLoadPresetClicked();
    FReply HandleRenamePresetClicked();
    FReply HandleDeletePresetClicked();
    TSharedRef<SWidget> GenerateSavedPresetWidget(TSharedPtr<FString> Item);
    void HandleSavedPresetSelectionChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo);
    FText GetSelectedSavedPresetText() const;

    void ApplyPresetIfNeeded(FName ChangedPropertyName);
    void RefreshViewport(bool bRefitCamera);
    void RefreshCurrentAsset(bool bRefitCamera);
    void RefreshSavedPresetOptions();
    void PersistLastUsedSettings();
    void SetCurrentAssetIndex(int32 NewIndex);
    void GenerateAssets(bool bGenerateAll);

    FEasyThumbnailGeneratorRenderSettings BuildRenderSettingsFromViewport(bool bUseExplicitCamera) const;
    FText GetCurrentAssetLabel() const;
    FText GetSelectionSummaryText() const;

private:
    TArray<FAssetData> SelectedAssets;
    int32 CurrentAssetIndex = 0;

    TStrongObjectPtr<UEasyThumbnailGeneratorSessionSettings> SessionSettings;
    TSharedPtr<SEasyThumbnailGeneratorViewport> ViewportWidget;
    TSharedPtr<IDetailsView> DetailsView;

    TArray<TSharedPtr<FString>> SavedPresetOptions;
    TSharedPtr<FString> SelectedSavedPreset;
    TSharedPtr<SComboBox<TSharedPtr<FString>>> SavedPresetComboBox;
    TSharedPtr<SEditableTextBox> PresetNameTextBox;

    bool bApplyingPreset = false;
};
