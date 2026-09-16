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
    static constexpr float MinimumCameraDistance = 100.0f;

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
