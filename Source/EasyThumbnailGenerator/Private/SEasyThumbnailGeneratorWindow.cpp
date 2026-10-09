#include "SEasyThumbnailGeneratorWindow.h"

#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EasyThumbnailGeneratorRenderer.h"
#include "EasyThumbnailGeneratorFraming.h"
#include "EasyThumbnailGeneratorGeometryFraming.h"
#include "EasyThumbnailGeneratorMaterialUtils.h"
#include "EasyThumbnailGeneratorPreviewLighting.h"
#include "EasyThumbnailGeneratorUserSettings.h"
#include "DrawDebugHelpers.h"
#include "DynamicMeshBuilder.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Framework/Notifications/NotificationManager.h"
#include "IDetailsView.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Misc/MessageDialog.h"
#include "Modules/ModuleManager.h"
#include "PreviewScene.h"
#include "PropertyEditorModule.h"
#include "RenderingThread.h"
#include "UObject/UnrealType.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "EasyThumbnailGeneratorWindow"

namespace EasyThumbnailGenerator
{
    static void ShowSuccessNotification(const FText& Text)
    {
        FNotificationInfo Info(Text);
        Info.bFireAndForget = true;
        Info.ExpireDuration = 4.0f;
        Info.bUseSuccessFailIcons = true;

        if (TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info))
        {
            Notification->SetCompletionState(SNotificationItem::CS_Success);
        }
    }
}

FEasyThumbnailGeneratorViewportClient::FEasyThumbnailGeneratorViewportClient(
    FPreviewScene& InPreviewScene,
    const TSharedRef<SEditorViewport>& InViewportWidget)
    : FEditorViewportClient(nullptr, &InPreviewScene, InViewportWidget)
    , ViewportWidget(InViewportWidget)
    , PreviewScene(InPreviewScene)
{
    SetViewLocation(FVector(-200.0f, 0.0f, 0.0f));
    SetViewRotation(FRotator::ZeroRotator);
    AddRealtimeOverride(true, LOCTEXT("RealtimeOverride", "Easy Thumbnail Generator"));
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
    Settings.bUseExplicitCameraTransform = true;
    Settings.ExplicitCameraLocation = GetViewLocation();
    Settings.ExplicitCameraRotation = GetViewRotation();
    Settings.ExplicitOrthoWidth = GetOrthoZoom();
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

void FEasyThumbnailGeneratorViewportClient::Draw(
    const FSceneView* View,
    FPrimitiveDrawInterface* PDI)
{
    FEditorViewportClient::Draw(View, PDI);

    if (!CurrentSettings.bShowCheckerboard || !View || !PDI || !GEngine)
    {
        return;
    }

    UMaterial* CheckerMaterial = GEngine->VertexColorViewModeMaterial_ColorOnly;
    if (!CheckerMaterial)
    {
        CheckerMaterial = GEngine->VertexColorMaterial;
    }

    if (!CheckerMaterial)
    {
        return;
    }

    FBoxSphereBounds AssetBounds;
    if (!GetAssetBounds(AssetBounds))
    {
        return;
    }

    const FVector ViewDirection = View->GetViewDirection().GetSafeNormal();
    const FVector ViewRight = View->GetViewRight().GetSafeNormal();
    const FVector ViewUp = View->GetViewUp().GetSafeNormal();

    if (ViewDirection.IsNearlyZero() || ViewRight.IsNearlyZero() || ViewUp.IsNearlyZero())
    {
        return;
    }

    const FBoxSphereBounds FrameRelativeBounds =
        EasyThumbnailGenerator::MakeFrameRelativeBounds(AssetBounds, CurrentSettings, 1.0f);

    const float HalfAssetDepth =
        EasyThumbnailGenerator::CalculateProjectedHalfSpan(ViewDirection, FrameRelativeBounds);

    const FVector ViewLocation = GetViewLocation();
    const float DistanceToFrameCenter = FMath::Max(
        FVector::DotProduct(-ViewLocation, ViewDirection),
        10.0f);

    // Place the checkerboard safely behind the entire framed asset. It is submitted as
    // an editor view element rather than scene geometry, so it cannot affect lighting,
    // reflections, shadows, or generated PNG output.
    const float BackgroundDistance =
        DistanceToFrameCenter + HalfAssetDepth + FMath::Max(50.0f, HalfAssetDepth * 0.25f);

    const FIntRect ViewRect = View->UnscaledViewRect;
    const float ViewportWidth = FMath::Max(1, ViewRect.Width());
    const float ViewportHeight = FMath::Max(1, ViewRect.Height());
    const float ViewportAspectRatio = ViewportWidth / ViewportHeight;

    float HalfBackgroundWidth = 500.0f;
    float HalfBackgroundHeight = 500.0f;

    if (View->IsPerspectiveProjection())
    {
        const float HorizontalFOVRadians =
            FMath::DegreesToRadians(FMath::Clamp(ViewFOV, 1.0f, 170.0f));

        HalfBackgroundWidth =
            BackgroundDistance * FMath::Tan(HorizontalFOVRadians * 0.5f) * 1.05f;
        HalfBackgroundHeight =
            (HalfBackgroundWidth / FMath::Max(ViewportAspectRatio, 0.01f)) * 1.05f;
    }
    else
    {
        HalfBackgroundWidth = FMath::Max(GetOrthoZoom() * 0.55f, 10.0f);
        HalfBackgroundHeight =
            HalfBackgroundWidth / FMath::Max(ViewportAspectRatio, 0.01f);
    }

    const FVector PlaneCenter = ViewLocation + ViewDirection * BackgroundDistance;
    const FVector BottomLeft =
        PlaneCenter - ViewRight * HalfBackgroundWidth - ViewUp * HalfBackgroundHeight;

    constexpr int32 CheckerColumns = 32;
    const float FullWidth = HalfBackgroundWidth * 2.0f;
    const float FullHeight = HalfBackgroundHeight * 2.0f;
    const float CellSize = FullWidth / static_cast<float>(CheckerColumns);
    const int32 CheckerRows = FMath::Max(1, FMath::CeilToInt(FullHeight / CellSize));

    FDynamicMeshBuilder MeshBuilder(View->GetFeatureLevel());
    MeshBuilder.ReserveVertices(CheckerColumns * CheckerRows * 4);
    MeshBuilder.ReserveTriangles(CheckerColumns * CheckerRows * 2);

    const FVector3f TangentX(ViewRight);
    const FVector3f TangentY(ViewUp);
    const FVector3f TangentZ(-ViewDirection);

    const FColor DarkChecker(72, 72, 72);
    const FColor LightChecker(120, 120, 120);

    for (int32 Y = 0; Y < CheckerRows; ++Y)
    {
        for (int32 X = 0; X < CheckerColumns; ++X)
        {
            const float X0 = X * CellSize;
            const float X1 = FMath::Min((X + 1) * CellSize, FullWidth);
            const float Y0 = Y * CellSize;
            const float Y1 = FMath::Min((Y + 1) * CellSize, FullHeight);

            const FColor CellColor = ((X + Y) & 1) == 0 ? DarkChecker : LightChecker;

            const FVector P0 = BottomLeft + ViewRight * X0 + ViewUp * Y0;
            const FVector P1 = BottomLeft + ViewRight * X1 + ViewUp * Y0;
            const FVector P2 = BottomLeft + ViewRight * X1 + ViewUp * Y1;
            const FVector P3 = BottomLeft + ViewRight * X0 + ViewUp * Y1;

            const int32 V0 = MeshBuilder.AddVertex(
                FVector3f(P0), FVector2f::ZeroVector, TangentX, TangentY, TangentZ, CellColor);
            const int32 V1 = MeshBuilder.AddVertex(
                FVector3f(P1), FVector2f::ZeroVector, TangentX, TangentY, TangentZ, CellColor);
            const int32 V2 = MeshBuilder.AddVertex(
                FVector3f(P2), FVector2f::ZeroVector, TangentX, TangentY, TangentZ, CellColor);
            const int32 V3 = MeshBuilder.AddVertex(
                FVector3f(P3), FVector2f::ZeroVector, TangentX, TangentY, TangentZ, CellColor);

            MeshBuilder.AddTriangle(V0, V1, V2);
            MeshBuilder.AddTriangle(V0, V2, V3);
        }
    }

    MeshBuilder.Draw(
        PDI,
        FMatrix::Identity,
        CheckerMaterial->GetRenderProxy(),
        SDPG_World,
        true,
        false,
        FHitProxyId());
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
            const FVector FrameCenter = EasyThumbnailGenerator::GetFramingCenter(Bounds, CurrentSettings);
            PreviewScene.AddComponent(PreviewComponent, FTransform(FQuat::Identity, -FrameCenter), false);
        }
        else
        {
            PreviewScene.AddComponent(PreviewComponent, FTransform::Identity, false);
        }

        PreviewComponent->SetCastShadow(true);
        if (UMeshComponent* MeshComponent = Cast<UMeshComponent>(PreviewComponent))
        {
            MeshComponent->SetTextureForceResidentFlag(true);
            MeshComponent->PrestreamTextures(30.0f, true, 0);
        }

        if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(PreviewComponent))
        {
            SkeletalMeshComponent->RefreshBoneTransforms();
            SkeletalMeshComponent->UpdateComponentToWorld();
        }

        PreviewComponent->MarkRenderStateDirty();
        PreviewComponent->UpdateBounds();
    }

    RefreshScene();
    FitCameraToAsset();
}

void FEasyThumbnailGeneratorViewportClient::ApplySettings(
    const FEasyThumbnailGeneratorRenderSettings& InSettings,
    bool bRefitCamera)
{
    CurrentSettings = InSettings;
    CurrentSettings.bUseExplicitCameraTransform = false;

    UpdatePreviewTransform();

    PreviewScene.SetLightBrightness(CurrentSettings.DirectionalLightIntensity);
    PreviewScene.SetLightDirection(FRotator(CurrentSettings.DirectionalLightPitch, CurrentSettings.DirectionalLightYaw, 0.0f));
    EasyThumbnailGenerator::ApplyPreviewSkyLightIntensity(PreviewScene, CurrentSettings.SkyLightIntensity);

    ExposureSettings.bFixed = CurrentSettings.bUseManualExposure;
    if (CurrentSettings.bUseManualExposure)
    {
        ExposureSettings.FixedEV100 = CurrentSettings.ExposureCompensation;
    }

    SyncViewToSettings(bRefitCamera);
    RefreshScene();
}

void FEasyThumbnailGeneratorViewportClient::FitCameraToAsset()
{
    CurrentSettings.CameraYaw = GetViewRotation().Yaw;
    CurrentSettings.CameraPitch = GetViewRotation().Pitch;
    CurrentSettings.PerspectiveFOV = ViewFOV;
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

void FEasyThumbnailGeneratorViewportClient::UpdatePreviewTransform()
{
    if (!PreviewComponent)
    {
        return;
    }

    FBoxSphereBounds Bounds;
    if (!GetAssetBounds(Bounds))
    {
        return;
    }

    const FVector FrameCenter = EasyThumbnailGenerator::GetFramingCenter(Bounds, CurrentSettings);
    PreviewComponent->SetWorldTransform(FTransform(FQuat::Identity, -FrameCenter));
    PreviewComponent->UpdateComponentToWorld();
    PreviewComponent->MarkRenderTransformDirty();
    PreviewComponent->UpdateBounds();
}

void FEasyThumbnailGeneratorViewportClient::RefreshBoundsVisualizer()
{
    UWorld* World = PreviewScene.GetWorld();
    if (!World)
    {
        return;
    }

    FlushPersistentDebugLines(World);

    if (!CurrentSettings.bShowBounds)
    {
        return;
    }

    FBoxSphereBounds Bounds;
    if (!GetAssetBounds(Bounds))
    {
        return;
    }

    const FVector FrameCenter = EasyThumbnailGenerator::GetFramingCenter(Bounds, CurrentSettings);
    const FVector RelativeBoundsCenter = Bounds.Origin - FrameCenter;

    DrawDebugBox(
        World,
        RelativeBoundsCenter,
        Bounds.BoxExtent,
        FColor::Green,
        true,
        -1.0f,
        0,
        1.0f);
}

void FEasyThumbnailGeneratorViewportClient::RefreshScene()
{
    RefreshBoundsVisualizer();

    if (PreviewScene.GetWorld())
    {
        PreviewScene.GetWorld()->SendAllEndOfFrameUpdates();
    }

    FlushRenderingCommands();
    Invalidate();
}

void FEasyThumbnailGeneratorViewportClient::SyncViewToSettings(bool bRefitCamera)
{
    if (!bRefitCamera)
    {
        Invalidate();
        return;
    }

    const FRotator CameraRotation(CurrentSettings.CameraPitch, CurrentSettings.CameraYaw, 0.0f);
    SetViewRotation(CameraRotation);
    ViewFOV = CurrentSettings.PerspectiveFOV;
    FOVAngle = CurrentSettings.PerspectiveFOV;

    FBoxSphereBounds Bounds;
    if (!GetAssetBounds(Bounds))
    {
        Invalidate();
        return;
    }

    float ViewportAspectRatio = 1.0f;
    if (const TSharedPtr<SEditorViewport> PinnedViewport = ViewportWidget.Pin())
    {
        const FVector2D ViewportSize = PinnedViewport->GetCachedGeometry().GetLocalSize();
        if (ViewportSize.X > 1.0f && ViewportSize.Y > 1.0f)
        {
            ViewportAspectRatio = ViewportSize.X / ViewportSize.Y;
        }
    }

    const float PaddingMultiplier =
        1.0f + (FMath::Max(CurrentSettings.FramePaddingPercent, 0.0f) * 0.01f);

    const FRotationMatrix CameraMatrix(CameraRotation);
    const FVector CameraForward = CameraMatrix.GetUnitAxis(EAxis::X);

    EasyThumbnailGenerator::FGeometryFitResult GeometryFit;
    if (EasyThumbnailGenerator::CalculateGeometryAwareFit(
            Asset.Get(),
            PreviewComponent,
            Bounds,
            CurrentSettings,
            CameraRotation,
            ViewportAspectRatio,
            PaddingMultiplier,
            10.0f,
            GeometryFit))
    {
        float CameraDistance = 10.0f;

        if (CurrentSettings.ProjectionMode == EEasyThumbnailGeneratorProjectionMode::Orthographic)
        {
            SetViewportType(LVT_OrthoFreelook);
            SetOrthoZoom(GeometryFit.OrthoWidth);
            CameraDistance = GeometryFit.OrthoCameraDistance;
        }
        else
        {
            SetViewportType(LVT_Perspective);
            CameraDistance = GeometryFit.PerspectiveDistance;
        }

        SetViewLocation(GeometryFit.ViewTarget - (CameraForward * CameraDistance));
        SetLookAtLocation(GeometryFit.ViewTarget);
        Invalidate();
        return;
    }

    // Fallback for assets whose CPU-side vertex data is unavailable.
    const FBoxSphereBounds FrameRelativeBounds =
        EasyThumbnailGenerator::MakeFrameRelativeBounds(Bounds, CurrentSettings, 1.0f);

    const FVector CameraRight = CameraMatrix.GetUnitAxis(EAxis::Y);
    const FVector CameraUp = CameraMatrix.GetUnitAxis(EAxis::Z);

    const float HalfWidth =
        EasyThumbnailGenerator::CalculateProjectedHalfSpan(CameraRight, FrameRelativeBounds);
    const float HalfHeight =
        EasyThumbnailGenerator::CalculateProjectedHalfSpan(CameraUp, FrameRelativeBounds);
    const float HalfDepth =
        EasyThumbnailGenerator::CalculateProjectedHalfSpan(CameraForward, FrameRelativeBounds);

    float CameraDistance = 10.0f;

    if (CurrentSettings.ProjectionMode == EEasyThumbnailGeneratorProjectionMode::Orthographic)
    {
        SetViewportType(LVT_OrthoFreelook);
        const float RequiredHalfWidth =
            FMath::Max(HalfWidth, HalfHeight * FMath::Max(ViewportAspectRatio, 0.01f));
        SetOrthoZoom(FMath::Max(RequiredHalfWidth * 2.0f * PaddingMultiplier, 2.0f));
        CameraDistance =
            FMath::Max(10.0f, (HalfDepth * PaddingMultiplier) + 10.0f);
    }
    else
    {
        SetViewportType(LVT_Perspective);

        const float HorizontalFOVRadians =
            FMath::DegreesToRadians(FMath::Max(1.0f, CurrentSettings.PerspectiveFOV));
        const float VerticalFOVRadians = 2.0f * FMath::Atan(
            FMath::Tan(HorizontalFOVRadians * 0.5f) /
            FMath::Max(ViewportAspectRatio, 0.01f));

        const float DistanceFromWidth =
            HalfWidth / FMath::Tan(HorizontalFOVRadians * 0.5f);
        const float DistanceFromHeight =
            HalfHeight / FMath::Tan(VerticalFOVRadians * 0.5f);

        CameraDistance = FMath::Max(
            10.0f,
            (FMath::Max(DistanceFromWidth, DistanceFromHeight) + HalfDepth) *
                PaddingMultiplier);
    }

    SetViewLocation(-CameraForward * CameraDistance);
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

    EasyThumbnailGenerator::InitializePreviewSkyLighting(*PreviewScene);

    SEditorViewport::Construct(SEditorViewport::FArguments());
}

void SEasyThumbnailGeneratorViewport::SetAsset(UObject* InAsset)
{
    if (ViewportClient.IsValid())
    {
        ViewportClient->SetAsset(InAsset);
    }
}

void SEasyThumbnailGeneratorViewport::ApplySettings(
    const FEasyThumbnailGeneratorRenderSettings& InSettings,
    bool bRefitCamera)
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
    return ViewportClient.IsValid()
        ? ViewportClient->GetCurrentRenderSettings()
        : FEasyThumbnailGeneratorRenderSettings();
}

TSharedRef<FEditorViewportClient> SEasyThumbnailGeneratorViewport::MakeEditorViewportClient()
{
    ViewportClient = MakeShared<FEasyThumbnailGeneratorViewportClient>(
        *PreviewScene,
        StaticCastSharedRef<SEditorViewport>(SharedThis(this)));
    return ViewportClient.ToSharedRef();
}

void SEasyThumbnailGeneratorViewport::BindCommands()
{
    SEditorViewport::BindCommands();
}

void SEasyThumbnailGeneratorWindow::Construct(const FArguments& InArgs)
{
    SelectedAssets = InArgs._SelectedAssets;
    CurrentAssetIndex = 0;

    SessionSettings.Reset(NewObject<UEasyThumbnailGeneratorSessionSettings>());
    if (UEasyThumbnailGeneratorUserSettings* UserSettings = GetMutableDefault<UEasyThumbnailGeneratorUserSettings>())
    {
        UserSettings->ApplyLastUsedTo(*SessionSettings.Get());
    }

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

    RefreshSavedPresetOptions();

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
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("PreviousAsset", "<"))
                    .ToolTipText(LOCTEXT("PreviousAssetTooltip", "Preview the previous selected asset."))
                    .IsEnabled_Lambda([this]() { return SelectedAssets.Num() > 1; })
                    .OnClicked(this, &SEasyThumbnailGeneratorWindow::HandlePreviousAssetClicked)
                ]
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                .Padding(8.0f, 0.0f)
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Text(this, &SEasyThumbnailGeneratorWindow::GetCurrentAssetLabel)
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("NextAsset", ">"))
                    .ToolTipText(LOCTEXT("NextAssetTooltip", "Preview the next selected asset."))
                    .IsEnabled_Lambda([this]() { return SelectedAssets.Num() > 1; })
                    .OnClicked(this, &SEasyThumbnailGeneratorWindow::HandleNextAssetClicked)
                ]
            ]

            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            .Padding(8.0f, 4.0f, 8.0f, 8.0f)
            [
                SAssignNew(ViewportWidget, SEasyThumbnailGeneratorViewport)
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(8.0f, 0.0f, 8.0f, 4.0f)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("CameraShortcutsLabel", "Camera"))
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(8.0f, 0.0f, 8.0f, 6.0f)
            [
                SNew(SUniformGridPanel)
                .SlotPadding(FMargin(2.0f))
                + SUniformGridPanel::Slot(0, 0)
                [
                    SNew(SButton).Text(LOCTEXT("CameraFront", "Front"))
                    .OnClicked_Lambda([this]() { return HandleCameraShortcut(180.0f, 0.0f); })
                ]
                + SUniformGridPanel::Slot(1, 0)
                [
                    SNew(SButton).Text(LOCTEXT("CameraBack", "Back"))
                    .OnClicked_Lambda([this]() { return HandleCameraShortcut(0.0f, 0.0f); })
                ]
                + SUniformGridPanel::Slot(2, 0)
                [
                    SNew(SButton).Text(LOCTEXT("CameraLeft", "Left"))
                    .OnClicked_Lambda([this]() { return HandleCameraShortcut(-90.0f, 0.0f); })
                ]
                + SUniformGridPanel::Slot(3, 0)
                [
                    SNew(SButton).Text(LOCTEXT("CameraRight", "Right"))
                    .OnClicked_Lambda([this]() { return HandleCameraShortcut(90.0f, 0.0f); })
                ]
                + SUniformGridPanel::Slot(0, 1)
                [
                    SNew(SButton).Text(LOCTEXT("CameraTop", "Top"))
                    .OnClicked_Lambda([this]() { return HandleCameraShortcut(0.0f, -89.0f); })
                ]
                + SUniformGridPanel::Slot(1, 1)
                [
                    SNew(SButton).Text(LOCTEXT("CameraBottom", "Bottom"))
                    .OnClicked_Lambda([this]() { return HandleCameraShortcut(0.0f, 89.0f); })
                ]
                + SUniformGridPanel::Slot(2, 1)
                [
                    SNew(SButton).Text(LOCTEXT("CameraThreeQuarterLeft", "3/4 Left"))
                    .OnClicked_Lambda([this]() { return HandleCameraShortcut(-135.0f, -10.0f); })
                ]
                + SUniformGridPanel::Slot(3, 1)
                [
                    SNew(SButton).Text(LOCTEXT("CameraThreeQuarterRight", "3/4 Right"))
                    .OnClicked_Lambda([this]() { return HandleCameraShortcut(135.0f, -10.0f); })
                ]
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
                .FillWidth(1.0f)
                [
                    SNew(SBox)
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(6.0f, 0.0f, 0.0f, 0.0f)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("GenerateCurrentButton", "Generate Current"))
                    .ToolTipText(LOCTEXT("GenerateCurrentTooltip", "Generate the current asset using the exact live preview camera framing."))
                    .OnClicked(this, &SEasyThumbnailGeneratorWindow::HandleGenerateCurrentClicked)
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(6.0f, 0.0f, 0.0f, 0.0f)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("GenerateAllButton", "Generate All"))
                    .ToolTipText(LOCTEXT("GenerateAllTooltip", "Generate all selected assets using the same camera direction/settings and fit each asset to frame."))
                    .Visibility_Lambda([this]() { return SelectedAssets.Num() > 1 ? EVisibility::Visible : EVisibility::Collapsed; })
                    .OnClicked(this, &SEasyThumbnailGeneratorWindow::HandleGenerateAllClicked)
                ]
            ]
        ]

        + SSplitter::Slot()
        .Value(0.28f)
        [
            SNew(SVerticalBox)

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(8.0f, 8.0f, 8.0f, 4.0f)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("SavedPresetsTitle", "Saved Presets"))
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(8.0f, 0.0f, 8.0f, 4.0f)
            [
                SAssignNew(SavedPresetComboBox, SComboBox<TSharedPtr<FString>>)
                .OptionsSource(&SavedPresetOptions)
                .OnGenerateWidget(this, &SEasyThumbnailGeneratorWindow::GenerateSavedPresetWidget)
                .OnSelectionChanged(this, &SEasyThumbnailGeneratorWindow::HandleSavedPresetSelectionChanged)
                [
                    SNew(STextBlock)
                    .Text(this, &SEasyThumbnailGeneratorWindow::GetSelectedSavedPresetText)
                ]
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(8.0f, 0.0f, 8.0f, 4.0f)
            [
                SAssignNew(PresetNameTextBox, SEditableTextBox)
                .HintText(LOCTEXT("PresetNameHint", "Preset name"))
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(8.0f, 0.0f, 8.0f, 8.0f)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                .Padding(0.0f, 0.0f, 4.0f, 0.0f)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("SavePresetButton", "Save"))
                    .OnClicked(this, &SEasyThumbnailGeneratorWindow::HandleSavePresetClicked)
                ]
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                .Padding(0.0f, 0.0f, 4.0f, 0.0f)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("LoadPresetButton", "Load"))
                    .IsEnabled_Lambda([this]() { return SelectedSavedPreset.IsValid(); })
                    .OnClicked(this, &SEasyThumbnailGeneratorWindow::HandleLoadPresetClicked)
                ]
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                .Padding(0.0f, 0.0f, 4.0f, 0.0f)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("RenamePresetButton", "Rename"))
                    .IsEnabled_Lambda([this]() { return SelectedSavedPreset.IsValid(); })
                    .OnClicked(this, &SEasyThumbnailGeneratorWindow::HandleRenamePresetClicked)
                ]
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("DeletePresetButton", "Delete"))
                    .IsEnabled_Lambda([this]() { return SelectedSavedPreset.IsValid(); })
                    .OnClicked(this, &SEasyThumbnailGeneratorWindow::HandleDeletePresetClicked)
                ]
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SSeparator)
            ]

            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            .Padding(8.0f)
            [
                SNew(SBorder)
                .Padding(6.0f)
                [
                    DetailsView.ToSharedRef()
                ]
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(8.0f, 0.0f, 8.0f, 8.0f)
            [
                SNew(STextBlock)
                .AutoWrapText(true)
                .Text(this, &SEasyThumbnailGeneratorWindow::GetSelectionSummaryText)
            ]
        ]
    ];

    if (ViewportWidget.IsValid() && !SelectedAssets.IsEmpty())
    {
        RefreshCurrentAsset(true);
    }
}

SEasyThumbnailGeneratorWindow::~SEasyThumbnailGeneratorWindow()
{
    PersistLastUsedSettings();
}

void SEasyThumbnailGeneratorWindow::HandleSettingsChanged(const FPropertyChangedEvent& PropertyChangedEvent)
{
    if (bApplyingPreset || !SessionSettings.IsValid())
    {
        return;
    }

    const FName ChangedPropertyName = PropertyChangedEvent.Property
        ? PropertyChangedEvent.Property->GetFName()
        : NAME_None;

    ApplyPresetIfNeeded(ChangedPropertyName);

    const bool bCameraRelatedChange =
        ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UEasyThumbnailGeneratorSessionSettings, Preset) ||
        ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UEasyThumbnailGeneratorSessionSettings, FramingMode) ||
        ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UEasyThumbnailGeneratorSessionSettings, FrameOffset) ||
        ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UEasyThumbnailGeneratorSessionSettings, ProjectionMode) ||
        ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UEasyThumbnailGeneratorSessionSettings, CameraYaw) ||
        ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UEasyThumbnailGeneratorSessionSettings, CameraPitch) ||
        ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UEasyThumbnailGeneratorSessionSettings, PerspectiveFOV) ||
        ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UEasyThumbnailGeneratorSessionSettings, FramePaddingPercent);

    RefreshViewport(bCameraRelatedChange);
    PersistLastUsedSettings();
}

FReply SEasyThumbnailGeneratorWindow::HandleGenerateCurrentClicked()
{
    GenerateAssets(false);
    return FReply::Handled();
}

FReply SEasyThumbnailGeneratorWindow::HandleGenerateAllClicked()
{
    GenerateAssets(true);
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
    const EEasyThumbnailGeneratorPreset PresetToApply = SessionSettings->Preset == EEasyThumbnailGeneratorPreset::Custom
        ? EEasyThumbnailGeneratorPreset::AssetDefault
        : SessionSettings->Preset;
    SessionSettings->ApplyPreset(PresetToApply);
    DetailsView->ForceRefresh();
    bApplyingPreset = false;

    RefreshViewport(true);
    PersistLastUsedSettings();
    return FReply::Handled();
}

FReply SEasyThumbnailGeneratorWindow::HandlePreviousAssetClicked()
{
    if (SelectedAssets.Num() > 1)
    {
        SetCurrentAssetIndex(CurrentAssetIndex - 1);
    }
    return FReply::Handled();
}

FReply SEasyThumbnailGeneratorWindow::HandleNextAssetClicked()
{
    if (SelectedAssets.Num() > 1)
    {
        SetCurrentAssetIndex(CurrentAssetIndex + 1);
    }
    return FReply::Handled();
}

FReply SEasyThumbnailGeneratorWindow::HandleCameraShortcut(float Yaw, float Pitch)
{
    if (!SessionSettings.IsValid())
    {
        return FReply::Handled();
    }

    bApplyingPreset = true;
    SessionSettings->Preset = EEasyThumbnailGeneratorPreset::Custom;
    SessionSettings->CameraYaw = Yaw;
    SessionSettings->CameraPitch = Pitch;
    DetailsView->ForceRefresh();
    bApplyingPreset = false;

    RefreshViewport(true);
    PersistLastUsedSettings();
    return FReply::Handled();
}

FReply SEasyThumbnailGeneratorWindow::HandleSavePresetClicked()
{
    if (!PresetNameTextBox.IsValid())
    {
        return FReply::Handled();
    }

    const FString PresetName = PresetNameTextBox->GetText().ToString().TrimStartAndEnd();
    if (PresetName.IsEmpty())
    {
        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("PresetNameRequired", "Enter a name before saving a preset."));
        return FReply::Handled();
    }

    FEasyThumbnailGeneratorRenderSettings Settings = BuildRenderSettingsFromViewport(false);
    if (UEasyThumbnailGeneratorUserSettings* UserSettings = GetMutableDefault<UEasyThumbnailGeneratorUserSettings>())
    {
        UserSettings->SavePreset(PresetName, Settings);
        RefreshSavedPresetOptions();

        if (TSharedPtr<FString>* FoundItem = SavedPresetOptions.FindByPredicate(
            [&PresetName](const TSharedPtr<FString>& Item)
            {
                return Item.IsValid() && Item->Equals(PresetName, ESearchCase::IgnoreCase);
            }))
        {
            SelectedSavedPreset = *FoundItem;
        }
        else
        {
            SelectedSavedPreset.Reset();
        }

        if (SavedPresetComboBox.IsValid() && SelectedSavedPreset.IsValid())
        {
            SavedPresetComboBox->SetSelectedItem(SelectedSavedPreset);
        }

        EasyThumbnailGenerator::ShowSuccessNotification(
            FText::Format(LOCTEXT("PresetSaved", "Saved preset: {0}"), FText::FromString(PresetName)));
    }

    return FReply::Handled();
}

FReply SEasyThumbnailGeneratorWindow::HandleLoadPresetClicked()
{
    if (!SelectedSavedPreset.IsValid() || !SessionSettings.IsValid())
    {
        return FReply::Handled();
    }

    FEasyThumbnailGeneratorRenderSettings Settings;
    if (UEasyThumbnailGeneratorUserSettings* UserSettings = GetMutableDefault<UEasyThumbnailGeneratorUserSettings>();
        UserSettings && UserSettings->FindSavedPreset(*SelectedSavedPreset, Settings))
    {
        bApplyingPreset = true;
        SessionSettings->ApplyRenderSettings(Settings, EEasyThumbnailGeneratorPreset::Custom);
        DetailsView->ForceRefresh();
        bApplyingPreset = false;
        RefreshViewport(true);
        PersistLastUsedSettings();
    }

    return FReply::Handled();
}

FReply SEasyThumbnailGeneratorWindow::HandleRenamePresetClicked()
{
    if (!SelectedSavedPreset.IsValid() || !PresetNameTextBox.IsValid())
    {
        return FReply::Handled();
    }

    const FString OldName = *SelectedSavedPreset;
    const FString NewName = PresetNameTextBox->GetText().ToString().TrimStartAndEnd();
    if (NewName.IsEmpty())
    {
        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("RenamePresetNameRequired", "Enter a new name before renaming the preset."));
        return FReply::Handled();
    }

    if (UEasyThumbnailGeneratorUserSettings* UserSettings = GetMutableDefault<UEasyThumbnailGeneratorUserSettings>();
        UserSettings && UserSettings->RenamePreset(OldName, NewName))
    {
        RefreshSavedPresetOptions();

        if (TSharedPtr<FString>* FoundItem = SavedPresetOptions.FindByPredicate(
            [&NewName](const TSharedPtr<FString>& Item)
            {
                return Item.IsValid() && Item->Equals(NewName, ESearchCase::IgnoreCase);
            }))
        {
            SelectedSavedPreset = *FoundItem;
            if (SavedPresetComboBox.IsValid())
            {
                SavedPresetComboBox->SetSelectedItem(SelectedSavedPreset);
            }
        }

        EasyThumbnailGenerator::ShowSuccessNotification(
            FText::Format(LOCTEXT("PresetRenamed", "Renamed preset to: {0}"), FText::FromString(NewName)));
    }
    else
    {
        FMessageDialog::Open(
            EAppMsgType::Ok,
            LOCTEXT("PresetRenameFailed", "Could not rename the preset. The target name may already be in use."));
    }

    return FReply::Handled();
}

FReply SEasyThumbnailGeneratorWindow::HandleDeletePresetClicked()
{
    if (!SelectedSavedPreset.IsValid())
    {
        return FReply::Handled();
    }

    const FString PresetName = *SelectedSavedPreset;
    if (UEasyThumbnailGeneratorUserSettings* UserSettings = GetMutableDefault<UEasyThumbnailGeneratorUserSettings>();
        UserSettings && UserSettings->DeletePreset(PresetName))
    {
        SelectedSavedPreset.Reset();
        RefreshSavedPresetOptions();
        EasyThumbnailGenerator::ShowSuccessNotification(
            FText::Format(LOCTEXT("PresetDeleted", "Deleted preset: {0}"), FText::FromString(PresetName)));
    }

    return FReply::Handled();
}

TSharedRef<SWidget> SEasyThumbnailGeneratorWindow::GenerateSavedPresetWidget(TSharedPtr<FString> Item)
{
    return SNew(STextBlock)
        .Text(Item.IsValid() ? FText::FromString(*Item) : FText::GetEmpty());
}

void SEasyThumbnailGeneratorWindow::HandleSavedPresetSelectionChanged(
    TSharedPtr<FString> Item,
    ESelectInfo::Type SelectInfo)
{
    SelectedSavedPreset = Item;
    if (PresetNameTextBox.IsValid() && Item.IsValid())
    {
        PresetNameTextBox->SetText(FText::FromString(*Item));
    }
}

FText SEasyThumbnailGeneratorWindow::GetSelectedSavedPresetText() const
{
    return SelectedSavedPreset.IsValid()
        ? FText::FromString(*SelectedSavedPreset)
        : LOCTEXT("NoSavedPresetSelected", "Select saved preset...");
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
        DetailsView->ForceRefresh();
        bApplyingPreset = false;
        return;
    }

    if (SessionSettings->Preset != EEasyThumbnailGeneratorPreset::Custom)
    {
        bApplyingPreset = true;
        SessionSettings->Preset = EEasyThumbnailGeneratorPreset::Custom;
        DetailsView->ForceRefresh();
        bApplyingPreset = false;
    }
}

void SEasyThumbnailGeneratorWindow::RefreshViewport(bool bRefitCamera)
{
    if (ViewportWidget.IsValid() && SessionSettings.IsValid())
    {
        ViewportWidget->ApplySettings(SessionSettings->MakeRenderSettings(), bRefitCamera);
    }
}

void SEasyThumbnailGeneratorWindow::RefreshCurrentAsset(bool bRefitCamera)
{
    if (!ViewportWidget.IsValid() || SelectedAssets.IsEmpty())
    {
        return;
    }

    CurrentAssetIndex = FMath::Clamp(CurrentAssetIndex, 0, SelectedAssets.Num() - 1);
    ViewportWidget->SetAsset(SelectedAssets[CurrentAssetIndex].GetAsset());
    RefreshViewport(bRefitCamera);
}

void SEasyThumbnailGeneratorWindow::RefreshSavedPresetOptions()
{
    SavedPresetOptions.Reset();

    TArray<FString> Names;
    if (const UEasyThumbnailGeneratorUserSettings* UserSettings = GetDefault<UEasyThumbnailGeneratorUserSettings>())
    {
        UserSettings->GetSavedPresetNames(Names);
    }

    for (const FString& Name : Names)
    {
        SavedPresetOptions.Add(MakeShared<FString>(Name));
    }

    if (SavedPresetComboBox.IsValid())
    {
        SavedPresetComboBox->RefreshOptions();
    }
}

void SEasyThumbnailGeneratorWindow::PersistLastUsedSettings()
{
    if (!SessionSettings.IsValid())
    {
        return;
    }

    FEasyThumbnailGeneratorRenderSettings Settings = BuildRenderSettingsFromViewport(false);
    EEasyThumbnailGeneratorPreset EffectivePreset = SessionSettings->Preset;

    const bool bViewportCameraDiffers =
        !FMath::IsNearlyEqual(Settings.CameraYaw, SessionSettings->CameraYaw, 0.01f) ||
        !FMath::IsNearlyEqual(Settings.CameraPitch, SessionSettings->CameraPitch, 0.01f) ||
        !FMath::IsNearlyEqual(Settings.PerspectiveFOV, SessionSettings->PerspectiveFOV, 0.01f);

    if (bViewportCameraDiffers)
    {
        EffectivePreset = EEasyThumbnailGeneratorPreset::Custom;
    }

    if (UEasyThumbnailGeneratorUserSettings* UserSettings = GetMutableDefault<UEasyThumbnailGeneratorUserSettings>())
    {
        UserSettings->SaveLastUsed(Settings, EffectivePreset);
    }
}

void SEasyThumbnailGeneratorWindow::SetCurrentAssetIndex(int32 NewIndex)
{
    if (SelectedAssets.IsEmpty())
    {
        return;
    }

    if (SessionSettings.IsValid() && ViewportWidget.IsValid())
    {
        const FEasyThumbnailGeneratorRenderSettings CurrentViewSettings = BuildRenderSettingsFromViewport(false);
        bApplyingPreset = true;
        SessionSettings->ApplyRenderSettings(CurrentViewSettings, EEasyThumbnailGeneratorPreset::Custom);
        if (DetailsView.IsValid())
        {
            DetailsView->ForceRefresh();
        }
        bApplyingPreset = false;
    }

    const int32 AssetCount = SelectedAssets.Num();
    CurrentAssetIndex = ((NewIndex % AssetCount) + AssetCount) % AssetCount;
    RefreshCurrentAsset(true);
    PersistLastUsedSettings();
}

void SEasyThumbnailGeneratorWindow::GenerateAssets(bool bGenerateAll)
{
    if (SelectedAssets.IsEmpty())
    {
        return;
    }

    int32 SuccessCount = 0;
    FString LastOutputPath;
    TArray<FString> FailureMessages;

    if (bGenerateAll)
    {
        FEasyThumbnailGeneratorRenderSettings Settings = BuildRenderSettingsFromViewport(false);
        Settings.bUseExplicitCameraTransform = false;

        for (const FAssetData& AssetData : SelectedAssets)
        {
            UObject* Asset = AssetData.GetAsset();
            if (!Asset)
            {
                continue;
            }

            FString OutputPath;
            FText Error;
            if (FEasyThumbnailGeneratorRenderer::GenerateThumbnail(Asset, Settings, OutputPath, Error))
            {
                ++SuccessCount;
                LastOutputPath = MoveTemp(OutputPath);
            }
            else
            {
                FailureMessages.Add(FString::Printf(TEXT("%s: %s"), *AssetData.AssetName.ToString(), *Error.ToString()));
            }
        }
    }
    else
    {
        const FAssetData& AssetData = SelectedAssets[CurrentAssetIndex];
        UObject* Asset = AssetData.GetAsset();
        if (Asset)
        {
            FEasyThumbnailGeneratorRenderSettings Settings = BuildRenderSettingsFromViewport(true);
            FString OutputPath;
            FText Error;
            if (FEasyThumbnailGeneratorRenderer::GenerateThumbnail(Asset, Settings, OutputPath, Error))
            {
                SuccessCount = 1;
                LastOutputPath = MoveTemp(OutputPath);
            }
            else
            {
                FailureMessages.Add(FString::Printf(TEXT("%s: %s"), *AssetData.AssetName.ToString(), *Error.ToString()));
            }
        }
    }

    if (SuccessCount > 0)
    {
        const FText NotificationText = SuccessCount == 1
            ? FText::Format(LOCTEXT("SingleThumbnailCreated", "PNG thumbnail created: {0}"), FText::FromString(LastOutputPath))
            : FText::Format(LOCTEXT("MultipleThumbnailsCreated", "Created {0} PNG thumbnails in Saved/Thumbnails."), FText::AsNumber(SuccessCount));
        EasyThumbnailGenerator::ShowSuccessNotification(NotificationText);
    }

    if (!FailureMessages.IsEmpty())
    {
        FMessageDialog::Open(
            EAppMsgType::Ok,
            FText::FromString(FString::Join(FailureMessages, TEXT("\n"))),
            LOCTEXT("GenerationFailureTitle", "Easy Thumbnail Generator"));
    }

    PersistLastUsedSettings();
}

FEasyThumbnailGeneratorRenderSettings SEasyThumbnailGeneratorWindow::BuildRenderSettingsFromViewport(bool bUseExplicitCamera) const
{
    FEasyThumbnailGeneratorRenderSettings Settings = SessionSettings.IsValid()
        ? SessionSettings->MakeRenderSettings()
        : FEasyThumbnailGeneratorRenderSettings();

    if (ViewportWidget.IsValid())
    {
        const FEasyThumbnailGeneratorRenderSettings ViewportSettings = ViewportWidget->GetCurrentRenderSettings();
        Settings.CameraYaw = ViewportSettings.CameraYaw;
        Settings.CameraPitch = ViewportSettings.CameraPitch;
        Settings.PerspectiveFOV = ViewportSettings.PerspectiveFOV;

        if (bUseExplicitCamera)
        {
            Settings.bUseExplicitCameraTransform = true;
            Settings.ExplicitCameraLocation = ViewportSettings.ExplicitCameraLocation;
            Settings.ExplicitCameraRotation = ViewportSettings.ExplicitCameraRotation;
            Settings.ExplicitOrthoWidth = ViewportSettings.ExplicitOrthoWidth;
        }
    }

    return Settings;
}

FText SEasyThumbnailGeneratorWindow::GetCurrentAssetLabel() const
{
    if (SelectedAssets.IsEmpty())
    {
        return LOCTEXT("NoAssetSelected", "No supported mesh selected.");
    }

    return FText::Format(
        LOCTEXT("CurrentAssetLabel", "{0} / {1}    {2}"),
        FText::AsNumber(CurrentAssetIndex + 1),
        FText::AsNumber(SelectedAssets.Num()),
        FText::FromName(SelectedAssets[CurrentAssetIndex].AssetName));
}

FText SEasyThumbnailGeneratorWindow::GetSelectionSummaryText() const
{
    if (SelectedAssets.Num() <= 1)
    {
        return LOCTEXT(
            "SingleSelectionSummary",
            "Generate Current uses the live viewport camera and writes a transparent PNG to Saved/Thumbnails.");
    }

    return FText::Format(
        LOCTEXT(
            "MultiSelectionSummary",
            "{0} assets selected. Generate Current preserves the live camera framing; Generate All fits each asset independently using the current camera direction and render settings."),
        FText::AsNumber(SelectedAssets.Num()));
}

#undef LOCTEXT_NAMESPACE
