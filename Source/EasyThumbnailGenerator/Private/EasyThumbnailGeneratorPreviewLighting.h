#pragma once

#include "Components/SkyLightComponent.h"
#include "Engine/TextureCube.h"
#include "PreviewScene.h"
#include "UObject/SoftObjectPtr.h"

namespace EasyThumbnailGenerator
{
    inline UTextureCube* GetDefaultPreviewSkyCubemap()
    {
        // UE 5.8's default Asset Viewer environment. A bare FPreviewScene otherwise
        // captures an empty/black world for its skylight, so changing intensity alone
        // has no visible effect.
        static TSoftObjectPtr<UTextureCube> DefaultSkyCubemap(
            FSoftObjectPath(TEXT("/Engine/EditorMaterials/AssetViewer/EpicQuadPanorama_CC+EV1.EpicQuadPanorama_CC+EV1")));

        return DefaultSkyCubemap.LoadSynchronous();
    }

    inline void InitializePreviewSkyLighting(FPreviewScene& PreviewScene)
    {
        if (UTextureCube* Cubemap = GetDefaultPreviewSkyCubemap())
        {
            PreviewScene.SetSkyCubemap(Cubemap);
        }

        PreviewScene.UpdateCaptureContents();
    }

    inline void ApplyPreviewSkyLightIntensity(FPreviewScene& PreviewScene, float Intensity)
    {
        const float ClampedIntensity = FMath::Max(Intensity, 0.0f);

        // Ensure the preview scene has a non-black environment source before scaling it.
        if (UTextureCube* Cubemap = GetDefaultPreviewSkyCubemap())
        {
            PreviewScene.SetSkyCubemap(Cubemap);
        }

        PreviewScene.SetSkyBrightness(ClampedIntensity);

        if (PreviewScene.SkyLight)
        {
            PreviewScene.SkyLight->SetIntensity(ClampedIntensity);
            PreviewScene.SkyLight->SetCaptureIsDirty();
        }

        PreviewScene.UpdateCaptureContents();
    }
}
