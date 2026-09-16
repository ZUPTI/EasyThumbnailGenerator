#include "EasyThumbnailGeneratorRenderer.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "HAL/FileManager.h"
#include "ImageCore.h"
#include "ImageUtils.h"
#include "Materials/MaterialInterface.h"
#include "Misc/Paths.h"
#include "PixelFormat.h"
#include "PreviewScene.h"
#include "RenderingThread.h"

#define LOCTEXT_NAMESPACE "EasyThumbnailGeneratorRenderer"

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

bool FEasyThumbnailGeneratorRenderer::GenerateThumbnail(
    UObject* Asset,
    const FEasyThumbnailGeneratorRenderSettings& Settings,
    FString& OutFilePath,
    FText& OutError)
{
    OutFilePath.Reset();
    OutError = FText::GetEmpty();

    if (!IsValid(Asset))
    {
        OutError = LOCTEXT("InvalidAsset", "The selected asset is invalid.");
        return false;
    }

    FBoxSphereBounds AssetBounds;
    if (!GetAssetBounds(Asset, AssetBounds))
    {
        OutError = LOCTEXT("UnsupportedAsset", "Only Static Mesh and Skeletal Mesh assets are supported.");
        return false;
    }

    TArray<FColor> Pixels;
    if (!RenderAsset(Asset, AssetBounds, Settings, Pixels, OutError))
    {
        return false;
    }

    const FString OutputDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Thumbnails"));
    if (!IFileManager::Get().MakeDirectory(*OutputDirectory, true) && !IFileManager::Get().DirectoryExists(*OutputDirectory))
    {
        OutError = FText::Format(
            LOCTEXT("CreateDirectoryFailed", "Could not create thumbnail output directory:\n{0}"),
            FText::FromString(OutputDirectory));
        return false;
    }

    OutFilePath = FPaths::Combine(OutputDirectory, Asset->GetName() + TEXT(".png"));

    const FImageView ImageView(
        Pixels.GetData(),
        Settings.OutputResolution,
        Settings.OutputResolution,
        1,
        ERawImageFormat::BGRA8,
        EGammaSpace::sRGB);
    if (!FImageUtils::SaveImageByExtension(*OutFilePath, ImageView, 100))
    {
        OutError = FText::Format(
            LOCTEXT("SaveFailed", "Failed to write PNG thumbnail:\n{0}"),
            FText::FromString(OutFilePath));
        OutFilePath.Reset();
        return false;
    }

    return true;
}

bool FEasyThumbnailGeneratorRenderer::GetAssetBounds(UObject* Asset, FBoxSphereBounds& OutBounds)
{
    if (const UStaticMesh* StaticMesh = Cast<UStaticMesh>(Asset))
    {
        OutBounds = StaticMesh->GetBounds();
        return true;
    }

    if (const USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(Asset))
    {
        OutBounds = SkeletalMesh->GetImportedBounds();
        return true;
    }

    return false;
}

bool FEasyThumbnailGeneratorRenderer::RenderAsset(
    UObject* Asset,
    const FBoxSphereBounds& AssetBounds,
    const FEasyThumbnailGeneratorRenderSettings& Settings,
    TArray<FColor>& OutPixels,
    FText& OutError)
{
    FPreviewScene PreviewScene(
        FPreviewScene::ConstructionValues()
            .SetCreatePhysicsScene(false)
            .ShouldSimulatePhysics(false)
            .AllowAudioPlayback(false)
            .SetTransactional(false)
            .SetEditor(true)
            .SetForceMipsResident(true));

    PreviewScene.SetLightBrightness(Settings.DirectionalLightIntensity);
    PreviewScene.SetLightDirection(FRotator(Settings.DirectionalLightPitch, Settings.DirectionalLightYaw, 0.0f));
    PreviewScene.SetSkyBrightness(Settings.SkyLightIntensity);

    UPrimitiveComponent* MeshComponent = nullptr;

    if (UStaticMesh* StaticMesh = Cast<UStaticMesh>(Asset))
    {
        UStaticMeshComponent* StaticMeshComponent = NewObject<UStaticMeshComponent>(GetTransientPackage());
        StaticMeshComponent->SetStaticMesh(StaticMesh);
        EasyThumbnailGenerator::ApplyStaticMeshMaterials(StaticMeshComponent, StaticMesh);
        StaticMeshComponent->SetMobility(EComponentMobility::Movable);
        StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        StaticMeshComponent->bCastDynamicShadow = true;
        StaticMeshComponent->bCastStaticShadow = true;
        MeshComponent = StaticMeshComponent;
    }
    else if (USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(Asset))
    {
        USkeletalMeshComponent* SkeletalMeshComponent = NewObject<USkeletalMeshComponent>(GetTransientPackage());
        SkeletalMeshComponent->SetSkeletalMeshAsset(SkeletalMesh);
        EasyThumbnailGenerator::ApplySkeletalMeshMaterials(SkeletalMeshComponent, SkeletalMesh);
        SkeletalMeshComponent->bForceRefpose = true;
        SkeletalMeshComponent->SetMobility(EComponentMobility::Movable);
        SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        SkeletalMeshComponent->bCastDynamicShadow = true;
        SkeletalMeshComponent->bCastStaticShadow = true;
        MeshComponent = SkeletalMeshComponent;
    }

    if (!MeshComponent)
    {
        OutError = LOCTEXT("MeshComponentFailed", "Failed to create a preview component for the selected asset.");
        return false;
    }

    MeshComponent->SetCastShadow(true);
    if (UMeshComponent* MeshComponentBase = Cast<UMeshComponent>(MeshComponent))
    {
        MeshComponentBase->SetTextureForceResidentFlag(true);
    }
    MeshComponent->UpdateBounds();

    const FTransform MeshTransform(FQuat::Identity, -AssetBounds.Origin);
    PreviewScene.AddComponent(MeshComponent, MeshTransform, false);

    if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(MeshComponent))
    {
        SkeletalMeshComponent->RefreshBoneTransforms();
        SkeletalMeshComponent->UpdateComponentToWorld();
    }

    MeshComponent->MarkRenderStateDirty();
    MeshComponent->UpdateBounds();
    if (UMeshComponent* MeshComponentBase = Cast<UMeshComponent>(MeshComponent))
    {
        MeshComponentBase->PrestreamTextures(30.0f, true, 0);
    }
    PreviewScene.GetWorld()->Tick(LEVELTICK_All, 0.0f);
    PreviewScene.GetWorld()->SendAllEndOfFrameUpdates();
    FlushRenderingCommands();
    PreviewScene.UpdateCaptureContents();

    const FVector SafeExtent(
        FMath::Max(AssetBounds.BoxExtent.X, MinimumBoundsExtent),
        FMath::Max(AssetBounds.BoxExtent.Y, MinimumBoundsExtent),
        FMath::Max(AssetBounds.BoxExtent.Z, MinimumBoundsExtent));

    const FBoxSphereBounds CenteredBounds(FVector::ZeroVector, SafeExtent, SafeExtent.Size());

    const float PaddingMultiplier = 1.0f + (FMath::Max(Settings.FramePaddingPercent, 0.0f) * 0.01f);

    FRotator CameraRotation(Settings.CameraPitch, Settings.CameraYaw, 0.0f);
    FVector CameraLocation = FVector::ZeroVector;
    float OrthoWidth = 0.0f;

    if (Settings.bUseExplicitCameraTransform)
    {
        CameraRotation = Settings.ExplicitCameraRotation;
        CameraLocation = Settings.ExplicitCameraLocation;
        OrthoWidth = FMath::Max(Settings.ExplicitOrthoWidth, 2.0f);
    }
    else
    {
        const FVector CameraForward = CameraRotation.Vector();
        float CameraDistance = MinimumCameraDistance;

        if (Settings.ProjectionMode == EEasyThumbnailGeneratorProjectionMode::Orthographic)
        {
            OrthoWidth = CalculateOrthoWidth(CenteredBounds, CameraRotation, PaddingMultiplier);

            const FRotationMatrix CameraMatrix(CameraRotation);
            const FVector CameraView = CameraMatrix.GetUnitAxis(EAxis::X);
            const float HalfDepth =
                FMath::Abs(CameraView.X) * SafeExtent.X +
                FMath::Abs(CameraView.Y) * SafeExtent.Y +
                FMath::Abs(CameraView.Z) * SafeExtent.Z;

            CameraDistance = FMath::Max(MinimumCameraDistance, (HalfDepth * PaddingMultiplier) + 100.0f);
        }
        else
        {
            CameraDistance = FMath::Max(
                MinimumCameraDistance,
                CalculatePerspectiveDistance(
                    CenteredBounds,
                    CameraRotation,
                    Settings.PerspectiveFOV,
                    Settings.PerspectiveFOV,
                    PaddingMultiplier));
        }

        CameraLocation = -CameraForward * CameraDistance;
    }

    UTextureRenderTarget2D* ColorTarget = NewObject<UTextureRenderTarget2D>(GetTransientPackage());
    ColorTarget->ClearColor = FLinearColor::Transparent;
    ColorTarget->InitCustomFormat(Settings.OutputResolution, Settings.OutputResolution, PF_B8G8R8A8, false);
    ColorTarget->UpdateResourceImmediate(true);

    UTextureRenderTarget2D* MaskTarget = NewObject<UTextureRenderTarget2D>(GetTransientPackage());
    MaskTarget->ClearColor = FLinearColor::Transparent;
    MaskTarget->InitCustomFormat(Settings.OutputResolution, Settings.OutputResolution, PF_FloatRGBA, true);
    MaskTarget->UpdateResourceImmediate(true);

    USceneCaptureComponent2D* CaptureComponent = NewObject<USceneCaptureComponent2D>(GetTransientPackage());
    CaptureComponent->bCaptureEveryFrame = false;
    CaptureComponent->bCaptureOnMovement = false;
    CaptureComponent->bAlwaysPersistRenderingState = false;
    CaptureComponent->CompositeMode = ESceneCaptureCompositeMode::SCCM_Overwrite;
    CaptureComponent->ProjectionType =
        Settings.ProjectionMode == EEasyThumbnailGeneratorProjectionMode::Orthographic
            ? ECameraProjectionMode::Orthographic
            : ECameraProjectionMode::Perspective;

    if (Settings.ProjectionMode == EEasyThumbnailGeneratorProjectionMode::Orthographic)
    {
        CaptureComponent->OrthoWidth = OrthoWidth;
        CaptureComponent->bAutoCalculateOrthoPlanes = true;
    }
    else
    {
        CaptureComponent->FOVAngle = Settings.PerspectiveFOV;
        CaptureComponent->bAutoCalculateOrthoPlanes = false;
    }

    CaptureComponent->ShowFlags.SetAtmosphere(false);
    CaptureComponent->ShowFlags.SetFog(false);
    CaptureComponent->ShowFlags.SetGrid(false);
    CaptureComponent->ShowFlags.SetSelectionOutline(false);
    CaptureComponent->ShowFlags.SetEditor(false);
    CaptureComponent->ShowFlags.SetCompositeEditorPrimitives(false);
    CaptureComponent->ShowFlags.SetTemporalAA(true);
    CaptureComponent->ShowFlags.SetAntiAliasing(true);
    CaptureComponent->ShowFlags.SetMaterials(true);
    CaptureComponent->ShowFlags.SetLighting(true);
    CaptureComponent->ShowFlags.SetPostProcessing(true);
    CaptureComponent->ShowFlags.SetTonemapper(true);
    CaptureComponent->ShowFlags.SetDeferredLighting(true);
    CaptureComponent->ShowFlags.SetReflectionEnvironment(true);

    if (Settings.bUseManualExposure)
    {
        CaptureComponent->PostProcessBlendWeight = 1.0f;
        CaptureComponent->PostProcessSettings.bOverride_AutoExposureMethod = true;
        CaptureComponent->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
        CaptureComponent->PostProcessSettings.bOverride_AutoExposureBias = true;
        CaptureComponent->PostProcessSettings.AutoExposureBias = Settings.ExposureCompensation;
        CaptureComponent->PostProcessSettings.bOverride_MotionBlurAmount = true;
        CaptureComponent->PostProcessSettings.MotionBlurAmount = 0.0f;
    }

    const FTransform CameraTransform(CameraRotation, CameraLocation);
    PreviewScene.AddComponent(CaptureComponent, CameraTransform, false);

    CaptureComponent->TextureTarget = ColorTarget;
    CaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
    CaptureComponent->CaptureScene();
    FlushRenderingCommands();

    FImage ColorImage;
    if (!FImageUtils::GetRenderTargetImage(ColorTarget, ColorImage))
    {
        OutError = LOCTEXT("ColorReadFailed", "Failed to read the color render target.");
        return false;
    }

    CaptureComponent->TextureTarget = MaskTarget;
    CaptureComponent->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;
    CaptureComponent->CaptureScene();
    FlushRenderingCommands();

    FImage MaskImage;
    if (!FImageUtils::GetRenderTargetImage(MaskTarget, MaskImage))
    {
        OutError = LOCTEXT("MaskReadFailed", "Failed to read the alpha render target.");
        return false;
    }

    ColorImage.ChangeFormat(ERawImageFormat::BGRA8, EGammaSpace::sRGB);
    MaskImage.ChangeFormat(ERawImageFormat::RGBA32F, EGammaSpace::Linear);

    const TArrayView64<const FColor> ColorPixels = ColorImage.AsBGRA8();
    const TArrayView64<const FLinearColor> MaskPixels = MaskImage.AsRGBA32F();
    const int64 ExpectedPixelCount = static_cast<int64>(Settings.OutputResolution) * static_cast<int64>(Settings.OutputResolution);

    if (ColorPixels.Num() != ExpectedPixelCount || MaskPixels.Num() != ExpectedPixelCount)
    {
        OutError = LOCTEXT("UnexpectedImageSize", "The render target returned an unexpected pixel count.");
        return false;
    }

    OutPixels.SetNumUninitialized(static_cast<int32>(ExpectedPixelCount));

    for (int64 PixelIndex = 0; PixelIndex < ExpectedPixelCount; ++PixelIndex)
    {
        const float Opacity = FMath::Clamp(1.0f - MaskPixels[PixelIndex].A, 0.0f, 1.0f);
        FColor Pixel = ColorPixels[PixelIndex];

        if (Opacity <= KINDA_SMALL_NUMBER)
        {
            Pixel.R = 0;
            Pixel.G = 0;
            Pixel.B = 0;
            Pixel.A = 0;
            OutPixels[PixelIndex] = Pixel;
            continue;
        }

        if (Opacity < 1.0f - KINDA_SMALL_NUMBER)
        {
            FLinearColor LinearColor(Pixel);
            LinearColor.R = FMath::Clamp(LinearColor.R / Opacity, 0.0f, 1.0f);
            LinearColor.G = FMath::Clamp(LinearColor.G / Opacity, 0.0f, 1.0f);
            LinearColor.B = FMath::Clamp(LinearColor.B / Opacity, 0.0f, 1.0f);
            Pixel = LinearColor.ToFColorSRGB();
        }

        Pixel.A = static_cast<uint8>(FMath::RoundToInt(Opacity * 255.0f));
        OutPixels[PixelIndex] = Pixel;
    }

    return true;
}

float FEasyThumbnailGeneratorRenderer::CalculateOrthoWidth(
    const FBoxSphereBounds& Bounds,
    const FRotator& CameraRotation,
    float PaddingMultiplier)
{
    const FRotationMatrix CameraMatrix(CameraRotation);
    const FVector CameraRight = CameraMatrix.GetUnitAxis(EAxis::Y);
    const FVector CameraUp = CameraMatrix.GetUnitAxis(EAxis::Z);
    const FVector Extent = Bounds.BoxExtent;

    const float ProjectedHalfWidth =
        FMath::Abs(CameraRight.X) * Extent.X +
        FMath::Abs(CameraRight.Y) * Extent.Y +
        FMath::Abs(CameraRight.Z) * Extent.Z;

    const float ProjectedHalfHeight =
        FMath::Abs(CameraUp.X) * Extent.X +
        FMath::Abs(CameraUp.Y) * Extent.Y +
        FMath::Abs(CameraUp.Z) * Extent.Z;

    const float RequiredHalfSpan = FMath::Max(ProjectedHalfWidth, ProjectedHalfHeight);
    return FMath::Max(RequiredHalfSpan * 2.0f * PaddingMultiplier, 2.0f);
}

float FEasyThumbnailGeneratorRenderer::CalculatePerspectiveDistance(
    const FBoxSphereBounds& Bounds,
    const FRotator& CameraRotation,
    float HorizontalFOVDegrees,
    float VerticalFOVDegrees,
    float PaddingMultiplier)
{
    const FRotationMatrix CameraMatrix(CameraRotation);
    const FVector CameraForward = CameraMatrix.GetUnitAxis(EAxis::X);
    const FVector CameraRight = CameraMatrix.GetUnitAxis(EAxis::Y);
    const FVector CameraUp = CameraMatrix.GetUnitAxis(EAxis::Z);
    const FVector Extent = Bounds.BoxExtent;

    const float HalfWidth =
        FMath::Abs(CameraRight.X) * Extent.X +
        FMath::Abs(CameraRight.Y) * Extent.Y +
        FMath::Abs(CameraRight.Z) * Extent.Z;

    const float HalfHeight =
        FMath::Abs(CameraUp.X) * Extent.X +
        FMath::Abs(CameraUp.Y) * Extent.Y +
        FMath::Abs(CameraUp.Z) * Extent.Z;

    const float HalfDepth =
        FMath::Abs(CameraForward.X) * Extent.X +
        FMath::Abs(CameraForward.Y) * Extent.Y +
        FMath::Abs(CameraForward.Z) * Extent.Z;

    const float HorizontalFOVRadians = FMath::DegreesToRadians(FMath::Max(1.0f, HorizontalFOVDegrees));
    const float VerticalFOVRadians = FMath::DegreesToRadians(FMath::Max(1.0f, VerticalFOVDegrees));

    const float DistanceFromWidth = HalfWidth / FMath::Tan(HorizontalFOVRadians * 0.5f);
    const float DistanceFromHeight = HalfHeight / FMath::Tan(VerticalFOVRadians * 0.5f);

    return (FMath::Max(DistanceFromWidth, DistanceFromHeight) + HalfDepth) * PaddingMultiplier;
}

#undef LOCTEXT_NAMESPACE
