#pragma once

#include "CoreMinimal.h"
#include "EasyThumbnailGeneratorSessionSettings.h"

namespace EasyThumbnailGenerator
{
    inline FVector GetFramingCenter(
        const FBoxSphereBounds& AssetBounds,
        const FEasyThumbnailGeneratorRenderSettings& Settings)
    {
        switch (Settings.FramingMode)
        {
        case EEasyThumbnailGeneratorFramingMode::Pivot:
            return FVector::ZeroVector;

        case EEasyThumbnailGeneratorFramingMode::CustomOffset:
            return AssetBounds.Origin + Settings.FrameOffset;

        case EEasyThumbnailGeneratorFramingMode::BoundsCenter:
        default:
            return AssetBounds.Origin;
        }
    }

    inline FBoxSphereBounds MakeFrameRelativeBounds(
        const FBoxSphereBounds& AssetBounds,
        const FEasyThumbnailGeneratorRenderSettings& Settings,
        float MinimumExtent = 1.0f)
    {
        const FVector SafeExtent(
            FMath::Max(AssetBounds.BoxExtent.X, MinimumExtent),
            FMath::Max(AssetBounds.BoxExtent.Y, MinimumExtent),
            FMath::Max(AssetBounds.BoxExtent.Z, MinimumExtent));

        const FVector FrameCenter = GetFramingCenter(AssetBounds, Settings);
        const FVector RelativeBoundsCenter = AssetBounds.Origin - FrameCenter;

        return FBoxSphereBounds(RelativeBoundsCenter, SafeExtent, SafeExtent.Size());
    }

    inline float CalculateProjectedHalfSpan(
        const FVector& Axis,
        const FBoxSphereBounds& FrameRelativeBounds)
    {
        const FVector Extent = FrameRelativeBounds.BoxExtent;

        const float CenterOffset = FMath::Abs(FVector::DotProduct(Axis, FrameRelativeBounds.Origin));
        const float ExtentProjection =
            FMath::Abs(Axis.X) * Extent.X +
            FMath::Abs(Axis.Y) * Extent.Y +
            FMath::Abs(Axis.Z) * Extent.Z;

        return CenterOffset + ExtentProjection;
    }
}
