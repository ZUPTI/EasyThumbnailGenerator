#pragma once

#include "CoreMinimal.h"
#include "EasyThumbnailGeneratorSessionSettings.h"
#include "UObject/Object.h"
#include "EasyThumbnailGeneratorUserSettings.generated.h"

USTRUCT()
struct FEasyThumbnailGeneratorSavedPreset
{
    GENERATED_BODY()

    UPROPERTY()
    FString Name;

    UPROPERTY()
    int32 OutputResolution = 1024;

    UPROPERTY()
    float FramePaddingPercent = 5.0f;

    UPROPERTY()
    EEasyThumbnailGeneratorProjectionMode ProjectionMode = EEasyThumbnailGeneratorProjectionMode::Perspective;

    UPROPERTY()
    float CameraYaw = -90.0f;

    UPROPERTY()
    float CameraPitch = 0.0f;

    UPROPERTY()
    float PerspectiveFOV = 30.0f;

    UPROPERTY()
    float DirectionalLightIntensity = 8.0f;

    UPROPERTY()
    float DirectionalLightYaw = -35.0f;

    UPROPERTY()
    float DirectionalLightPitch = -45.0f;

    UPROPERTY()
    float SkyLightIntensity = 1.0f;

    UPROPERTY()
    bool bUseManualExposure = true;

    UPROPERTY()
    float ExposureCompensation = 1.0f;

    void FromRenderSettings(const FEasyThumbnailGeneratorRenderSettings& Settings);
    FEasyThumbnailGeneratorRenderSettings ToRenderSettings() const;
};

UCLASS(config=EditorPerProjectUserSettings)
class EASYTHUMBNAILGENERATOR_API UEasyThumbnailGeneratorUserSettings : public UObject
{
    GENERATED_BODY()

public:
    UEasyThumbnailGeneratorUserSettings();

    void ApplyLastUsedTo(UEasyThumbnailGeneratorSessionSettings& SessionSettings) const;
    void SaveLastUsed(const FEasyThumbnailGeneratorRenderSettings& Settings, EEasyThumbnailGeneratorPreset Preset);

    void GetSavedPresetNames(TArray<FString>& OutNames) const;
    bool FindSavedPreset(const FString& Name, FEasyThumbnailGeneratorRenderSettings& OutSettings) const;
    void SavePreset(const FString& Name, const FEasyThumbnailGeneratorRenderSettings& Settings);
    bool RenamePreset(const FString& OldName, const FString& NewName);
    bool DeletePreset(const FString& Name);

private:
    UPROPERTY(Config)
    bool bHasLastUsedSettings = false;

    UPROPERTY(Config)
    EEasyThumbnailGeneratorPreset LastPreset = EEasyThumbnailGeneratorPreset::WeaponSide;

    UPROPERTY(Config)
    FEasyThumbnailGeneratorSavedPreset LastUsedSettings;

    UPROPERTY(Config)
    TArray<FEasyThumbnailGeneratorSavedPreset> SavedPresets;
};
