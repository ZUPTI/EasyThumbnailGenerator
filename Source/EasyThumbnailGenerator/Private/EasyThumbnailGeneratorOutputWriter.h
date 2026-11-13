#pragma once

#include "CoreMinimal.h"
#include "EasyThumbnailGeneratorSessionSettings.h"

class UObject;

namespace EasyThumbnailGenerator
{
    // Writes one rendered BGRA8 image to the selected destination(s).
    // If Skip Existing detects a collision, both targets are skipped together.
    bool WriteThumbnailOutputs(
        UObject* Asset,
        const FEasyThumbnailGeneratorRenderSettings& Settings,
        const TArray<FColor>& Pixels,
        FString& OutLocation,
        bool& bOutSkipped,
        FText& OutError);
}
