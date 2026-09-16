#include "SEasyThumbnailGeneratorWindow.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/MeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ContentBrowserModule.h"
#include "DetailLayoutBuilder.h"
#include "EasyThumbnailGeneratorRenderer.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Framework/Notifications/NotificationManager.h"
#include "HAL/FileManager.h"
#include "IDetailsView.h"
#include "ImageUtils.h"
#include "Materials/MaterialInterface.h"
#include "Misc/MessageDialog.h"
#include "Modules/ModuleManager.h"
#include "PreviewScene.h"
#include "PropertyEditorModule.h"
#include "RenderingThread.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "EasyThumbnailGeneratorWindow"

namespace EasyThumbnailGenerator
{
    static void ApplyStaticMeshMaterials(UStaticMeshComponent* MeshComponent, const UStaticMesh* StaticMesh)
    {
        if (!MeshComponent || !StaticMesh)
        {
            return;
        }

        const TArray<FStaticMaterial>& Materials = StaticMesh->GetStaticMaterials();
        for (int32 MaterialIndex = 0; MaterialIndex < Materials.Num(); ++MaterialIndex)
        {
            if (UMaterialInterface* Material = Materials[MaterialIndex].MaterialInterface)
            {
                MeshComponent->SetMaterial(MaterialIndex, Material);
            }
        }
    }

    static void ApplySkeletalMeshMaterials(USkeletalMeshComponent* MeshComponent, const USkeletalMesh* SkeletalMesh)
    {
        if (!MeshComponent || !SkeletalMesh)
        {
            return;
        }

        const TArray<FSkeletalMaterial>& Materials = SkeletalMesh->GetMaterials();
        for (int32 MaterialIndex = 0; MaterialIndex < Materials.Num(); ++MaterialIndex)
        {
            if (UMaterialInterface* Material = Materials[MaterialIndex].MaterialInterface)
            {
                MeshComponent->SetMaterial(MaterialIndex, Material);
            }
        }
    }
}

FEasyThumbnailGeneratorViewportClient::FEasyThumbnailGeneratorViewportClient(FPreviewScene& InPreviewScene, const TSharedRef<SEditorViewport>& InViewportWidget)
    : FEditorViewportClient(nullptr, &InPreviewScene, InViewportWidget)
    , PreviewScene(InPreviewScene)
{
    SetViewLocation(FVector(-200.0f, 0.0f, 0.0f));
    SetViewRotation(FRotator::ZeroRotator);
    SetRealtime(true, false);
    bSetListenerPosition = false;
    bUsingOrbitCamera = true;
    EngineShowFlags.SetSelection(false);
    EngineShowFlags.SetGrid(false);
    EngineShowFlags.SetCompositeEditorPrimitives(false);
}


FEasyThumbnailGeneratorRenderSettings FEasyThumbnailGeneratorViewportClient::GetCurrentRenderSettings() const
{
    FEasyThumbnailGeneratorRenderSettings Settings = CurrentSettings;
    Settings.CameraYaw = GetViewRotation().Yaw;
    Settings.CameraPitch = GetViewRotation().Pitch;
    Settings.PerspectiveFOV = ViewFOV;
    return Settings;
}

void FEasyThumbnailGeneratorViewportClient::Tick(float DeltaSeconds)
{
    FEditorViewportClient::Tick(DeltaSeconds);

    if (PreviewScene.GetWorld())
    {
        PreviewScene.GetWorld()->Tick(LEVELTICK_All, DeltaSeconds);
    }
}

void FEasyThumbnailGeneratorViewportClient::SetAsset(UObject* InAsset)
{
    ClearPreviewComponent();
    Asset = InAsset;

    if (!IsValid(InAsset))
    {
        return;
    }

    PreviewComponent = BuildPreviewComponent(InAsset);
    if (PreviewComponent)
    {
        FBoxSphereBounds Bounds;
        if (GetAssetBounds(Bounds))
        {
            const FTransform MeshTransform(FQuat::Identity, -Bounds.Origin);
            PreviewScene.AddComponent(PreviewComponent, MeshTransform, false);
        }
        else
        {
            PreviewScene.AddComponent(PreviewComponent, FTransform::Identity, false);
        }

        PreviewComponent->SetCastShadow(true);
        if (UMeshComponent* MeshComponentBase = Cast<UMeshComponent>(PreviewComponent))
        {
            MeshComponentBase->SetTextureForceResidentFlag(true);
        }
        PreviewComponent->MarkRenderStateDirty();
        PreviewComponent->UpdateBounds();
        if (UMeshComponent* MeshComponentBase = Cast<UMeshComponent>(PreviewComponent))
        {
            MeshComponentBase->PrestreamTextures(30.0f, true, 0);
        }
        if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(PreviewComponent))
        {
            SkeletalMeshComponent->RefreshBoneTransforms();
            SkeletalMeshComponent->UpdateComponentToWorld();
        }
    }

    RefreshScene();
    FitCameraToAsset();
}

void FEasyThumbnailGeneratorViewportClient::ApplySettings(const FEasyThumbnailGeneratorRenderSettings& InSettings, bool bRefitCamera)
{
    CurrentSettings = InSettings;
    PreviewScene.SetLightBrightness(CurrentSettings.DirectionalLightIntensity);
    PreviewScene.SetLightDirection(FRotator(CurrentSettings.DirectionalLightPitch, CurrentSettings.DirectionalLightYaw, 0.0f));
    PreviewScene.SetSkyBrightness(CurrentSettings.SkyLightIntensity);

    if (CurrentSettings.bUseManualExposure)
    {
        ExposureSettings.bFixed = true;
        ExposureSettings.FixedEV100 = CurrentSettings.ExposureCompensation;
    }
    else
    {
        ExposureSettings.bFixed = false;
    }

    SyncViewToSettings(bRefitCamera);
    RefreshScene();
}

void FEasyThumbnailGeneratorViewportClient::FitCameraToAsset()
{
    SyncViewToSettings(true);
}

bool FEasyThumbnailGeneratorViewportClient::GetAssetBounds(FBoxSphereBounds& OutBounds) const
{
    if (const UStaticMesh* StaticMesh = Cast<UStaticMesh>(Asset.Get()))
    {
        OutBounds = StaticMesh->GetBounds();
        return true;
    }

    if (const USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(Asset.Get()))
    {
        OutBounds = SkeletalMesh->GetImportedBounds();
        return true;
    }

    return false;
}

UPrimitiveComponent* FEasyThumbnailGeneratorViewportClient::BuildPreviewComponent(UObject* InAsset) const
{
    if (UStaticMesh* StaticMesh = Cast<UStaticMesh>(InAsset))
    {
        UStaticMeshComponent* StaticMeshComponent = NewObject<UStaticMeshComponent>(GetTransientPackage());
        StaticMeshComponent->SetStaticMesh(StaticMesh);
        EasyThumbnailGenerator::ApplyStaticMeshMaterials(StaticMeshComponent, StaticMesh);
        StaticMeshComponent->SetMobility(EComponentMobility::Movable);
        StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        return StaticMeshComponent;
    }

    if (USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(InAsset))
    {
        USkeletalMeshComponent* SkeletalMeshComponent = NewObject<USkeletalMeshComponent>(GetTransientPackage());
        SkeletalMeshComponent->SetSkeletalMeshAsset(SkeletalMesh);
        EasyThumbnailGenerator::ApplySkeletalMeshMaterials(SkeletalMeshComponent, SkeletalMesh);
        SkeletalMeshComponent->bForceRefpose = true;
        SkeletalMeshComponent->SetMobility(EComponentMobility::Movable);
        SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        return SkeletalMeshComponent;
    }

    return nullptr;
}

void FEasyThumbnailGeneratorViewportClient::ClearPreviewComponent()
{
    if (PreviewComponent)
    {
        PreviewScene.RemoveComponent(PreviewComponent);
        PreviewComponent = nullptr;
    }
}

void FEasyThumbnailGeneratorViewportClient::RefreshScene()
{
    if (PreviewScene.GetWorld())
    {
        PreviewScene.GetWorld()->SendAllEndOfFrameUpdates();
    }
    FlushRenderingCommands();
    Invalidate();
}

void FEasyThumbnailGeneratorViewportClient::SyncViewToSettings(bool bRefitCamera)
{
    const FRotator CameraRotation(CurrentSettings.CameraPitch, CurrentSettings.CameraYaw, 0.0f);
    SetViewRotation(CameraRotation);
    ViewFOV = CurrentSettings.PerspectiveFOV;
    FOVAngle = CurrentSettings.PerspectiveFOV;

    if (!bRefitCamera)
    {
        Invalidate();
        return;
    }

    FBoxSphereBounds Bounds;
    if (!GetAssetBounds(Bounds))
    {
        Invalidate();
        return;
    }

    const FVector SafeExtent(
        FMath::Max(Bounds.BoxExtent.X, 1.0f),
        FMath::Max(Bounds.BoxExtent.Y, 1.0f),
        FMath::Max(Bounds.BoxExtent.Z, 1.0f));

    const float PaddingMultiplier = 1.0f + (FMath::Max(CurrentSettings.FramePaddingPercent, 0.0f) * 0.01f);
    const FRotationMatrix CameraMatrix(CameraRotation);
    const FVector CameraForward = CameraMatrix.GetUnitAxis(EAxis::X);
    const FVector CameraRight = CameraMatrix.GetUnitAxis(EAxis::Y);
    const FVector CameraUp = CameraMatrix.GetUnitAxis(EAxis::Z);

    const float HalfWidth =
        FMath::Abs(CameraRight.X) * SafeExtent.X +
        FMath::Abs(CameraRight.Y) * SafeExtent.Y +
        FMath::Abs(CameraRight.Z) * SafeExtent.Z;

    const float HalfHeight =
        FMath::Abs(CameraUp.X) * SafeExtent.X +
        FMath::Abs(CameraUp.Y) * SafeExtent.Y +
        FMath::Abs(CameraUp.Z) * SafeExtent.Z;

    const float HalfDepth =
        FMath::Abs(CameraForward.X) * SafeExtent.X +
        FMath::Abs(CameraForward.Y) * SafeExtent.Y +
        FMath::Abs(CameraForward.Z) * SafeExtent.Z;

    float CameraDistance = 100.0f;
    if (CurrentSettings.ProjectionMode == EEasyThumbnailGeneratorProjectionMode::Orthographic)
    {
        SetViewportType(LVT_OrthoFreelook);
        const float OrthoWidth = FMath::Max(FMath::Max(HalfWidth, HalfHeight) * 2.0f * PaddingMultiplier, 2.0f);
        SetOrthoZoom(OrthoWidth);
        CameraDistance = FMath::Max(100.0f, HalfDepth * PaddingMultiplier + 100.0f);
    }
    else
    {
        const float FOVRadians = FMath::DegreesToRadians(FMath::Max(1.0f, CurrentSettings.PerspectiveFOV));
        const float DistanceFromWidth = HalfWidth / FMath::Tan(FOVRadians * 0.5f);
        const float DistanceFromHeight = HalfHeight / FMath::Tan(FOVRadians * 0.5f);
        CameraDistance = FMath::Max(100.0f, (FMath::Max(DistanceFromWidth, DistanceFromHeight) + HalfDepth) * PaddingMultiplier);
        SetViewportType(LVT_Perspective);
    }

    const FVector CameraLocation = -CameraForward * CameraDistance;
    SetViewLocation(CameraLocation);
    SetLookAtLocation(FVector::ZeroVector);
    Invalidate();
}

void SEasyThumbnailGeneratorViewport::Construct(const FArguments& InArgs)
{
    PreviewScene = MakeUnique<FPreviewScene>(
        FPreviewScene::ConstructionValues()
            .SetCreatePhysicsScene(false)
            .ShouldSimulatePhysics(false)
            .AllowAudioPlayback(false)
            .SetTransactional(false)
            .SetEditor(true)
            .SetForceMipsResident(true));

    SEditorViewport::Construct(SEditorViewport::FArguments());
}

void SEasyThumbnailGeneratorViewport::SetAsset(UObject* InAsset)
{
    if (ViewportClient.IsValid())
    {
        ViewportClient->SetAsset(InAsset);
    }
}

void SEasyThumbnailGeneratorViewport::ApplySettings(const FEasyThumbnailGeneratorRenderSettings& InSettings, bool bRefitCamera)
{
    if (ViewportClient.IsValid())
    {
        ViewportClient->ApplySettings(InSettings, bRefitCamera);
    }
}

void SEasyThumbnailGeneratorViewport::FitCameraToAsset()
{
    if (ViewportClient.IsValid())
    {
        ViewportClient->FitCameraToAsset();
    }
}

FEasyThumbnailGeneratorRenderSettings SEasyThumbnailGeneratorViewport::GetCurrentRenderSettings() const
{
    return ViewportClient.IsValid() ? ViewportClient->GetCurrentRenderSettings() : FEasyThumbnailGeneratorRenderSettings();
}

TSharedRef<FEditorViewportClient> SEasyThumbnailGeneratorViewport::MakeEditorViewportClient()
{
    ViewportClient = MakeShared<FEasyThumbnailGeneratorViewportClient>(*PreviewScene, StaticCastSharedRef<SEditorViewport>(SharedThis(this)));
    return ViewportClient.ToSharedRef();
}

void SEasyThumbnailGeneratorViewport::BindCommands()
{
    SEditorViewport::BindCommands();
}

void SEasyThumbnailGeneratorWindow::Construct(const FArguments& InArgs)
{
    SelectedAssets = InArgs._SelectedAssets;
    SessionSettings.Reset(NewObject<UEasyThumbnailGeneratorSessionSettings>());

    FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

    FDetailsViewArgs DetailsArgs;
    DetailsArgs.bAllowSearch = false;
    DetailsArgs.bHideSelectionTip = true;
    DetailsArgs.bLockable = false;
    DetailsArgs.bUpdatesFromSelection = false;
    DetailsArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
    DetailsView = PropertyEditorModule.CreateDetailView(DetailsArgs);
    DetailsView->SetObject(SessionSettings.Get());
    DetailsView->OnFinishedChangingProperties().AddSP(this, &SEasyThumbnailGeneratorWindow::HandleSettingsChanged);

    ChildSlot
    [
        SNew(SSplitter)
        + SSplitter::Slot()
        .Value(0.72f)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(8.0f, 8.0f, 8.0f, 4.0f)
            [
                SNew(STextBlock)
                .Text(GetSelectionLabel())
            ]
            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            .Padding(8.0f, 4.0f, 8.0f, 8.0f)
            [
                SAssignNew(ViewportWidget, SEasyThumbnailGeneratorViewport)
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(8.0f, 0.0f, 8.0f, 8.0f)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(0.0f, 0.0f, 6.0f, 0.0f)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("ResetButton", "Reset Preset"))
                    .OnClicked(this, &SEasyThumbnailGeneratorWindow::HandleResetClicked)
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(0.0f, 0.0f, 6.0f, 0.0f)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("FitButton", "Fit to Frame"))
                    .OnClicked(this, &SEasyThumbnailGeneratorWindow::HandleFitClicked)
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                [
                    SNew(SButton)
                    .Text(LOCTEXT("GenerateButton", "Generate PNG"))
                    .OnClicked(this, &SEasyThumbnailGeneratorWindow::HandleGenerateClicked)
                ]
            ]
        ]
        + SSplitter::Slot()
        .Value(0.28f)
        [
            SNew(SBorder)
            .Padding(8.0f)
            [
                DetailsView.ToSharedRef()
            ]
        ]
    ];

    if (ViewportWidget.IsValid() && !SelectedAssets.IsEmpty())
    {
        ViewportWidget->SetAsset(SelectedAssets[0].GetAsset());
        RefreshViewport(true);
    }
}

void SEasyThumbnailGeneratorWindow::HandleSettingsChanged(const FPropertyChangedEvent& PropertyChangedEvent)
{
    if (bApplyingPreset || !SessionSettings.IsValid())
    {
        return;
    }

    const FName ChangedPropertyName =
        PropertyChangedEvent.Property
            ? PropertyChangedEvent.Property->GetFName()
            : NAME_None;

    ApplyPresetIfNeeded(ChangedPropertyName);
    RefreshViewport(ChangedPropertyName != NAME_None);
}

FReply SEasyThumbnailGeneratorWindow::HandleGenerateClicked()
{
    FEasyThumbnailGeneratorRenderSettings RenderSettings = BuildRenderSettingsFromViewport();

    int32 SuccessCount = 0;
    FString LastOutputPath;
    TArray<FString> FailureMessages;

    for (const FAssetData& AssetData : SelectedAssets)
    {
        UObject* Asset = AssetData.GetAsset();
        if (!Asset)
        {
            continue;
        }

        FString OutputPath;
        FText Error;
        if (FEasyThumbnailGeneratorRenderer::GenerateThumbnail(Asset, RenderSettings, OutputPath, Error))
        {
            ++SuccessCount;
            LastOutputPath = MoveTemp(OutputPath);
        }
        else
        {
            FailureMessages.Add(FString::Printf(TEXT("%s: %s"), *AssetData.AssetName.ToString(), *Error.ToString()));
        }
    }

    if (SuccessCount > 0)
    {
        const FText NotificationText = SuccessCount == 1
            ? FText::Format(LOCTEXT("SingleThumbnailCreated", "PNG thumbnail created: {0}"), FText::FromString(LastOutputPath))
            : FText::Format(LOCTEXT("MultipleThumbnailsCreated", "Created {0} PNG thumbnails in Saved/Thumbnails."), FText::AsNumber(SuccessCount));

        FNotificationInfo Info(NotificationText);
        Info.bFireAndForget = true;
        Info.ExpireDuration = 5.0f;
        Info.bUseSuccessFailIcons = true;
        if (TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info))
        {
            Notification->SetCompletionState(SNotificationItem::CS_Success);
        }
    }

    if (!FailureMessages.IsEmpty())
    {
        FMessageDialog::Open(
            EAppMsgType::Ok,
            FText::FromString(FString::Join(FailureMessages, TEXT("\n"))),
            LOCTEXT("GenerationFailureTitle", "Easy Thumbnail Generator"));
    }

    return FReply::Handled();
}

FReply SEasyThumbnailGeneratorWindow::HandleFitClicked()
{
    if (ViewportWidget.IsValid())
    {
        ViewportWidget->FitCameraToAsset();
    }
    return FReply::Handled();
}

FReply SEasyThumbnailGeneratorWindow::HandleResetClicked()
{
    if (!SessionSettings.IsValid())
    {
        return FReply::Handled();
    }

    bApplyingPreset = true;
    SessionSettings->ApplyPreset(SessionSettings->Preset);
    if (DetailsView.IsValid())
    {
        DetailsView->ForceRefresh();
    }
    bApplyingPreset = false;
    RefreshViewport(true);
    return FReply::Handled();
}

void SEasyThumbnailGeneratorWindow::ApplyPresetIfNeeded(FName ChangedPropertyName)
{
    if (!SessionSettings.IsValid())
    {
        return;
    }

    if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UEasyThumbnailGeneratorSessionSettings, Preset))
    {
        bApplyingPreset = true;
        SessionSettings->ApplyPreset(SessionSettings->Preset);
        if (DetailsView.IsValid())
        {
            DetailsView->ForceRefresh();
        }
        bApplyingPreset = false;
        return;
    }

    if (SessionSettings->Preset != EEasyThumbnailGeneratorPreset::Custom)
    {
        SessionSettings->Preset = EEasyThumbnailGeneratorPreset::Custom;
        if (DetailsView.IsValid())
        {
            DetailsView->ForceRefresh();
        }
    }
}

void SEasyThumbnailGeneratorWindow::RefreshViewport(bool bRefitCamera)
{
    if (ViewportWidget.IsValid() && SessionSettings.IsValid())
    {
        ViewportWidget->ApplySettings(SessionSettings->MakeRenderSettings(), bRefitCamera);
    }
}

FEasyThumbnailGeneratorRenderSettings SEasyThumbnailGeneratorWindow::BuildRenderSettingsFromViewport() const
{
    FEasyThumbnailGeneratorRenderSettings Settings = SessionSettings.IsValid()
        ? SessionSettings->MakeRenderSettings()
        : FEasyThumbnailGeneratorRenderSettings();

    if (ViewportWidget.IsValid())
    {
        const FEasyThumbnailGeneratorRenderSettings ViewportSettings = ViewportWidget->GetCurrentRenderSettings();
        Settings.CameraYaw = ViewportSettings.CameraYaw;
        Settings.CameraPitch = ViewportSettings.CameraPitch;
    }

    return Settings;
}

FText SEasyThumbnailGeneratorWindow::GetSelectionLabel() const
{
    if (SelectedAssets.Num() == 1)
    {
        return FText::Format(LOCTEXT("SingleSelection", "Previewing: {0}"), FText::FromName(SelectedAssets[0].AssetName));
    }

    return FText::Format(LOCTEXT("MultipleSelection", "Previewing first selection. Generate applies to {0} assets."), FText::AsNumber(SelectedAssets.Num()));
}

#undef LOCTEXT_NAMESPACE
