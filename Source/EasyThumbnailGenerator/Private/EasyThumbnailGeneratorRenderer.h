#pragma once

#include "CoreMinimal.h"
#include "EasyThumbnailGeneratorSessionSettings.h"

class UObject;

class FEasyThumbnailGeneratorRenderer
{
public:
    static bool GenerateThumbnail(
        UObject* Asset,
        const FEasyThumbnailGeneratorRenderSettings& Settings,
        FString& OutFilePath,
        FText& OutError);

private:
    static constexpr float MinimumBoundsExtent = 1.0f;
    // Keep only a small near-plane safety margin. A 100uu floor made small assets
    // (for example small Quixel rocks) appear far too small after Fit to Frame.
    static constexpr float MinimumCameraDistance = 10.0f;

    static bool GetAssetBounds(UObject* Asset, FBoxSphereBounds& OutBounds);
    static bool RenderAsset(
        UObject* Asset,
        const FBoxSphereBounds& AssetBounds,
        const FEasyThumbnailGeneratorRenderSettings& Settings,
        TArray<FColor>& OutPixels,
        FText& OutError);

    static float CalculateOrthoWidth(const FBoxSphereBounds& Bounds, const FRotator& CameraRotation, float PaddingMultiplier);
    static float CalculatePerspectiveDistance(
        const FBoxSphereBounds& Bounds,
        const FRotator& CameraRotation,
        float HorizontalFOVDegrees,
        float VerticalFOVDegrees,
        float PaddingMultiplier);
};
