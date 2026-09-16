#include "EasyThumbnailGeneratorSessionSettings.h"

UEasyThumbnailGeneratorSessionSettings::UEasyThumbnailGeneratorSessionSettings()
{
    ApplyPreset(EEasyThumbnailGeneratorPreset::WeaponSide);
    Preset = EEasyThumbnailGeneratorPreset::WeaponSide;
    OutputResolution = 1024;
    FramePaddingPercent = 5.0f;
}

void UEasyThumbnailGeneratorSessionSettings::ApplyPreset(EEasyThumbnailGeneratorPreset InPreset)
{
    Preset = InPreset;

    switch (InPreset)
    {
    case EEasyThumbnailGeneratorPreset::WeaponSide:
        ProjectionMode = EEasyThumbnailGeneratorProjectionMode::Perspective;
        CameraYaw = -90.0f;
        CameraPitch = 0.0f;
        PerspectiveFOV = 25.0f;
        DirectionalLightIntensity = 8.0f;
        DirectionalLightYaw = -35.0f;
        DirectionalLightPitch = -45.0f;
        SkyLightIntensity = 1.0f;
        bUseManualExposure = true;
        ExposureCompensation = 1.0f;
        break;

    case EEasyThumbnailGeneratorPreset::WeaponThreeQuarter:
        ProjectionMode = EEasyThumbnailGeneratorProjectionMode::Perspective;
        CameraYaw = -125.0f;
        CameraPitch = -8.0f;
        PerspectiveFOV = 25.0f;
        DirectionalLightIntensity = 8.0f;
        DirectionalLightYaw = -35.0f;
        DirectionalLightPitch = -45.0f;
        SkyLightIntensity = 1.0f;
        bUseManualExposure = true;
        ExposureCompensation = 1.0f;
        break;

    case EEasyThumbnailGeneratorPreset::AssetDefault:
        ProjectionMode = EEasyThumbnailGeneratorProjectionMode::Perspective;
        CameraYaw = -135.0f;
        CameraPitch = -15.0f;
        PerspectiveFOV = 30.0f;
        DirectionalLightIntensity = 7.0f;
        DirectionalLightYaw = -35.0f;
        DirectionalLightPitch = -45.0f;
        SkyLightIntensity = 1.0f;
        bUseManualExposure = true;
        ExposureCompensation = 1.0f;
        break;

    case EEasyThumbnailGeneratorPreset::Custom:
    default:
        break;
    }
}


void UEasyThumbnailGeneratorSessionSettings::ApplyRenderSettings(
    const FEasyThumbnailGeneratorRenderSettings& Settings,
    EEasyThumbnailGeneratorPreset InPreset)
{
    Preset = InPreset;
    OutputResolution = Settings.OutputResolution;
    FramePaddingPercent = Settings.FramePaddingPercent;
    ProjectionMode = Settings.ProjectionMode;
    CameraYaw = Settings.CameraYaw;
    CameraPitch = Settings.CameraPitch;
    PerspectiveFOV = Settings.PerspectiveFOV;
    DirectionalLightIntensity = Settings.DirectionalLightIntensity;
    DirectionalLightYaw = Settings.DirectionalLightYaw;
    DirectionalLightPitch = Settings.DirectionalLightPitch;
    SkyLightIntensity = Settings.SkyLightIntensity;
    bUseManualExposure = Settings.bUseManualExposure;
    ExposureCompensation = Settings.ExposureCompensation;
}

FEasyThumbnailGeneratorRenderSettings UEasyThumbnailGeneratorSessionSettings::MakeRenderSettings() const
{
    FEasyThumbnailGeneratorRenderSettings Settings;
    Settings.OutputResolution = OutputResolution;
    Settings.FramePaddingPercent = FramePaddingPercent;
    Settings.ProjectionMode = ProjectionMode;
    Settings.CameraYaw = CameraYaw;
    Settings.CameraPitch = CameraPitch;
    Settings.PerspectiveFOV = PerspectiveFOV;
    Settings.DirectionalLightIntensity = DirectionalLightIntensity;
    Settings.DirectionalLightYaw = DirectionalLightYaw;
    Settings.DirectionalLightPitch = DirectionalLightPitch;
    Settings.SkyLightIntensity = SkyLightIntensity;
    Settings.bUseManualExposure = bUseManualExposure;
    Settings.ExposureCompensation = ExposureCompensation;
    return Settings;
}
