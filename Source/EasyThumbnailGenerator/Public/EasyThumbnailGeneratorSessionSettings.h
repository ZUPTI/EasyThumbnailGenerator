#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "EasyThumbnailGeneratorSessionSettings.generated.h"

UENUM()
enum class EEasyThumbnailGeneratorProjectionMode : uint8
{
    Perspective UMETA(DisplayName = "Perspective"),
    Orthographic UMETA(DisplayName = "Orthographic")
};

UENUM()
enum class EEasyThumbnailGeneratorPreset : uint8
{
    WeaponSide UMETA(DisplayName = "Weapon Side"),
    WeaponThreeQuarter UMETA(DisplayName = "Weapon 3/4"),
    AssetDefault UMETA(DisplayName = "Asset Default"),
    Custom UMETA(DisplayName = "Custom")
};

struct FEasyThumbnailGeneratorRenderSettings
{
    int32 OutputResolution = 1024;
    float FramePaddingPercent = 5.0f;
    EEasyThumbnailGeneratorProjectionMode ProjectionMode = EEasyThumbnailGeneratorProjectionMode::Perspective;
    float CameraYaw = -90.0f;
    float CameraPitch = 0.0f;
    float PerspectiveFOV = 30.0f;
    float DirectionalLightIntensity = 8.0f;
    float DirectionalLightYaw = -35.0f;
    float DirectionalLightPitch = -45.0f;
    float SkyLightIntensity = 1.0f;
    bool bUseManualExposure = true;
    float ExposureCompensation = 1.0f;
};

UCLASS(Transient)
class EASYTHUMBNAILGENERATOR_API UEasyThumbnailGeneratorSessionSettings : public UObject
{
    GENERATED_BODY()

public:
    UEasyThumbnailGeneratorSessionSettings();

    UPROPERTY(EditAnywhere, Category="Preset")
    EEasyThumbnailGeneratorPreset Preset;

    UPROPERTY(EditAnywhere, Category="Output", meta=(ClampMin="64", ClampMax="4096", UIMin="64", UIMax="4096"))
    int32 OutputResolution;

    UPROPERTY(EditAnywhere, Category="Output", meta=(ClampMin="0.0", ClampMax="50.0", UIMin="0.0", UIMax="50.0"))
    float FramePaddingPercent;

    UPROPERTY(EditAnywhere, Category="Camera")
    EEasyThumbnailGeneratorProjectionMode ProjectionMode;

    UPROPERTY(EditAnywhere, Category="Camera", meta=(ClampMin="-180.0", ClampMax="180.0", UIMin="-180.0", UIMax="180.0"))
    float CameraYaw;

    UPROPERTY(EditAnywhere, Category="Camera", meta=(ClampMin="-89.0", ClampMax="89.0", UIMin="-89.0", UIMax="89.0"))
    float CameraPitch;

    UPROPERTY(EditAnywhere, Category="Camera", meta=(ClampMin="5.0", ClampMax="120.0", UIMin="5.0", UIMax="120.0", EditCondition="ProjectionMode == EEasyThumbnailGeneratorProjectionMode::Perspective", EditConditionHides))
    float PerspectiveFOV;

    UPROPERTY(EditAnywhere, Category="Lighting", meta=(ClampMin="0.0", ClampMax="50.0", UIMin="0.0", UIMax="50.0"))
    float DirectionalLightIntensity;

    UPROPERTY(EditAnywhere, Category="Lighting", meta=(ClampMin="-180.0", ClampMax="180.0", UIMin="-180.0", UIMax="180.0"))
    float DirectionalLightYaw;

    UPROPERTY(EditAnywhere, Category="Lighting", meta=(ClampMin="-89.0", ClampMax="89.0", UIMin="-89.0", UIMax="89.0"))
    float DirectionalLightPitch;

    UPROPERTY(EditAnywhere, Category="Lighting", meta=(ClampMin="0.0", ClampMax="10.0", UIMin="0.0", UIMax="10.0"))
    float SkyLightIntensity;

    UPROPERTY(EditAnywhere, Category="Post Process")
    bool bUseManualExposure;

    UPROPERTY(EditAnywhere, Category="Post Process", meta=(ClampMin="-10.0", ClampMax="10.0", UIMin="-10.0", UIMax="10.0", EditCondition="bUseManualExposure", EditConditionHides))
    float ExposureCompensation;

    void ApplyPreset(EEasyThumbnailGeneratorPreset InPreset);
    FEasyThumbnailGeneratorRenderSettings MakeRenderSettings() const;
};
