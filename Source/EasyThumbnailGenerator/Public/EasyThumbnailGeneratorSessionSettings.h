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
enum class EEasyThumbnailGeneratorOutputMode : uint8
{
    PNG UMETA(DisplayName = "PNG"),
    TextureAsset UMETA(DisplayName = "Texture Asset"),
    Both UMETA(DisplayName = "Both")
};

UENUM()
enum class EEasyThumbnailGeneratorOverwritePolicy : uint8
{
    AutoIncrement UMETA(DisplayName = "Auto Increment"),
    Overwrite UMETA(DisplayName = "Overwrite"),
    SkipExisting UMETA(DisplayName = "Skip Existing")
};

UENUM()
enum class EEasyThumbnailGeneratorFramingMode : uint8
{
    VisibleContent UMETA(DisplayName = "Visible Content"),
    BoundsCenter UMETA(DisplayName = "Bounds Center"),
    Pivot UMETA(DisplayName = "Pivot"),
    CustomOffset UMETA(DisplayName = "Custom Offset")
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
    EEasyThumbnailGeneratorOutputMode OutputMode = EEasyThumbnailGeneratorOutputMode::PNG;
    EEasyThumbnailGeneratorOverwritePolicy OverwritePolicy = EEasyThumbnailGeneratorOverwritePolicy::AutoIncrement;
    FString PNGOutputDirectory = TEXT("Saved/Thumbnails");
    FString TextureAssetDirectory = TEXT("/Game/Thumbnails");
    FString SourcePrefixToStrip;
    FString OutputPrefix;
    FString FilenameTemplate = TEXT("{Prefix}{BaseName}");
    float FramePaddingPercent = 5.0f;
    EEasyThumbnailGeneratorFramingMode FramingMode = EEasyThumbnailGeneratorFramingMode::VisibleContent;
    FVector FrameOffset = FVector::ZeroVector;
    bool bShowBounds = false;
    bool bShowCheckerboard = false;
    EEasyThumbnailGeneratorProjectionMode ProjectionMode = EEasyThumbnailGeneratorProjectionMode::Perspective;
    float CameraYaw = -90.0f;
    float CameraPitch = 0.0f;
    float PerspectiveFOV = 30.0f;
    float DirectionalLightIntensity = 8.0f;
    float DirectionalLightYaw = -35.0f;
    float DirectionalLightPitch = -45.0f;
    float SkyLightIntensity = 1.0f;
    bool bUseManualExposure = true;
    // Fixed editor-style EV100 used by both the live preview and final capture.
    float ExposureCompensation = 1.0f;

    // When true, generation uses the exact viewport camera instead of auto fitting.
    // This is used by Generate Current to keep the PNG framing close to the live preview.
    bool bUseExplicitCameraTransform = false;
    FVector ExplicitCameraLocation = FVector::ZeroVector;
    FRotator ExplicitCameraRotation = FRotator::ZeroRotator;
    float ExplicitOrthoWidth = 512.0f;
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

    UPROPERTY(EditAnywhere, Category="Output", meta=(DisplayName="Output Mode"))
    EEasyThumbnailGeneratorOutputMode OutputMode;

    UPROPERTY(EditAnywhere, Category="Output", meta=(DisplayName="Overwrite Policy"))
    EEasyThumbnailGeneratorOverwritePolicy OverwritePolicy;

    UPROPERTY(EditAnywhere, Category="Output", meta=(DisplayName="PNG Directory", EditCondition="OutputMode != EEasyThumbnailGeneratorOutputMode::TextureAsset", ToolTip="Absolute path or directory relative to the project root. Default: Saved/Thumbnails."))
    FString PNGOutputDirectory;

    UPROPERTY(EditAnywhere, Category="Output", meta=(DisplayName="Texture Asset Directory", EditCondition="OutputMode != EEasyThumbnailGeneratorOutputMode::PNG", ToolTip="Long package path under /Game, for example /Game/UI/Thumbnails."))
    FString TextureAssetDirectory;

    UPROPERTY(EditAnywhere, Category="Output Naming", meta=(DisplayName="Source Prefix to Remove", ToolTip="Remove this exact leading prefix before constructing the output name. Example: SM_Rock with SM_ becomes Rock."))
    FString SourcePrefixToStrip;

    UPROPERTY(EditAnywhere, Category="Output Naming", meta=(DisplayName="Output Prefix", ToolTip="Prepended via the {Prefix} filename token. Example: T_."))
    FString OutputPrefix;

    UPROPERTY(EditAnywhere, Category="Output Naming", meta=(DisplayName="Filename Template", ToolTip="Tokens: {AssetName} (original), {BaseName} (after stripping source prefix), {Prefix} (new prefix). Example: {Prefix}{BaseName} outputs T_Rock from SM_Rock."))
    FString FilenameTemplate;

    UPROPERTY(EditAnywhere, Category="Output", meta=(ClampMin="0.0", ClampMax="50.0", UIMin="0.0", UIMax="50.0"))
    float FramePaddingPercent;

    UPROPERTY(EditAnywhere, Category="Framing", meta=(ToolTip="Visible Content uses projected mesh geometry to center the framing. Bounds Center, Pivot and Custom Offset keep their explicit framing anchors while still using geometry-aware fit extents."))
    EEasyThumbnailGeneratorFramingMode FramingMode;

    UPROPERTY(EditAnywhere, Category="Framing", meta=(EditCondition="FramingMode == EEasyThumbnailGeneratorFramingMode::CustomOffset", EditConditionHides, ToolTip="Offsets the framing center from the asset bounds center in asset-local units."))
    FVector FrameOffset;

    UPROPERTY(EditAnywhere, Category="Framing", meta=(DisplayName="Show Bounds", ToolTip="Draw the asset bounds in the live preview. This does not affect the exported PNG."))
    bool bShowBounds;

    UPROPERTY(EditAnywhere, Category="Preview", meta=(DisplayName="Checkerboard Background", ToolTip="Show Unreal Editor's alpha checkerboard behind the live preview. This does not affect the exported PNG."))
    bool bShowCheckerboard;

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

    UPROPERTY(EditAnywhere, Category="Post Process", meta=(DisplayName="Fixed EV100", ClampMin="-6.0", ClampMax="12.0", UIMin="-6.0", UIMax="12.0", EditCondition="bUseManualExposure", EditConditionHides, ToolTip="Fixed EV100 used consistently by the live preview and exported PNG."))
    float ExposureCompensation;

    void ApplyPreset(EEasyThumbnailGeneratorPreset InPreset);
    void ApplyRenderSettings(const FEasyThumbnailGeneratorRenderSettings& Settings, EEasyThumbnailGeneratorPreset InPreset = EEasyThumbnailGeneratorPreset::Custom);
    FEasyThumbnailGeneratorRenderSettings MakeRenderSettings() const;
};
