#pragma once

#include "CoreMinimal.h"
#include "EasyThumbnailGeneratorSessionSettings.h"

class UObject;
class UPrimitiveComponent;

namespace EasyThumbnailGenerator
{
    struct FGeometryFitResult
    {
        bool bValid = false;
        FVector ViewTarget = FVector::ZeroVector;
        float PerspectiveDistance = 0.0f;
        float OrthoWidth = 0.0f;
        float OrthoCameraDistance = 0.0f;
    };

    bool CalculateGeometryAwareFit(
        UObject* Asset,
        UPrimitiveComponent* MeshComponent,
        const FBoxSphereBounds& AssetBounds,
        const FEasyThumbnailGeneratorRenderSettings& Settings,
        const FRotator& CameraRotation,
        float ViewportAspectRatio,
        float PaddingMultiplier,
        float MinimumCameraDistance,
        FGeometryFitResult& OutResult);
}
