#include "EasyThumbnailGeneratorUserSettings.h"

void FEasyThumbnailGeneratorSavedPreset::FromRenderSettings(const FEasyThumbnailGeneratorRenderSettings& Settings)
{
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

FEasyThumbnailGeneratorRenderSettings FEasyThumbnailGeneratorSavedPreset::ToRenderSettings() const
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

UEasyThumbnailGeneratorUserSettings::UEasyThumbnailGeneratorUserSettings()
{
    LastUsedSettings.Name = TEXT("Last Used");
}

void UEasyThumbnailGeneratorUserSettings::ApplyLastUsedTo(UEasyThumbnailGeneratorSessionSettings& SessionSettings) const
{
    if (!bHasLastUsedSettings)
    {
        return;
    }

    SessionSettings.ApplyRenderSettings(LastUsedSettings.ToRenderSettings(), LastPreset);
}

void UEasyThumbnailGeneratorUserSettings::SaveLastUsed(
    const FEasyThumbnailGeneratorRenderSettings& Settings,
    EEasyThumbnailGeneratorPreset Preset)
{
    bHasLastUsedSettings = true;
    LastPreset = Preset;
    LastUsedSettings.Name = TEXT("Last Used");
    LastUsedSettings.FromRenderSettings(Settings);
    SaveConfig();
}

void UEasyThumbnailGeneratorUserSettings::GetSavedPresetNames(TArray<FString>& OutNames) const
{
    OutNames.Reset();
    OutNames.Reserve(SavedPresets.Num());

    for (const FEasyThumbnailGeneratorSavedPreset& Preset : SavedPresets)
    {
        if (!Preset.Name.IsEmpty())
        {
            OutNames.Add(Preset.Name);
        }
    }

    OutNames.Sort([](const FString& A, const FString& B)
    {
        return A.Compare(B, ESearchCase::IgnoreCase) < 0;
    });
}

bool UEasyThumbnailGeneratorUserSettings::FindSavedPreset(
    const FString& Name,
    FEasyThumbnailGeneratorRenderSettings& OutSettings) const
{
    const FEasyThumbnailGeneratorSavedPreset* FoundPreset = SavedPresets.FindByPredicate(
        [&Name](const FEasyThumbnailGeneratorSavedPreset& Preset)
        {
            return Preset.Name.Equals(Name, ESearchCase::IgnoreCase);
        });

    if (!FoundPreset)
    {
        return false;
    }

    OutSettings = FoundPreset->ToRenderSettings();
    return true;
}

void UEasyThumbnailGeneratorUserSettings::SavePreset(
    const FString& Name,
    const FEasyThumbnailGeneratorRenderSettings& Settings)
{
    const FString TrimmedName = Name.TrimStartAndEnd();
    if (TrimmedName.IsEmpty())
    {
        return;
    }

    FEasyThumbnailGeneratorSavedPreset* ExistingPreset = SavedPresets.FindByPredicate(
        [&TrimmedName](const FEasyThumbnailGeneratorSavedPreset& Preset)
        {
            return Preset.Name.Equals(TrimmedName, ESearchCase::IgnoreCase);
        });

    if (!ExistingPreset)
    {
        ExistingPreset = &SavedPresets.AddDefaulted_GetRef();
    }

    ExistingPreset->Name = TrimmedName;
    ExistingPreset->FromRenderSettings(Settings);
    SaveConfig();
}


bool UEasyThumbnailGeneratorUserSettings::RenamePreset(const FString& OldName, const FString& NewName)
{
    const FString TrimmedNewName = NewName.TrimStartAndEnd();
    if (OldName.IsEmpty() || TrimmedNewName.IsEmpty())
    {
        return false;
    }

    FEasyThumbnailGeneratorSavedPreset* ExistingPreset = SavedPresets.FindByPredicate(
        [&OldName](const FEasyThumbnailGeneratorSavedPreset& Preset)
        {
            return Preset.Name.Equals(OldName, ESearchCase::IgnoreCase);
        });

    if (!ExistingPreset)
    {
        return false;
    }

    const bool bNameAlreadyUsed = SavedPresets.ContainsByPredicate(
        [&TrimmedNewName, ExistingPreset](const FEasyThumbnailGeneratorSavedPreset& Preset)
        {
            return &Preset != ExistingPreset && Preset.Name.Equals(TrimmedNewName, ESearchCase::IgnoreCase);
        });

    if (bNameAlreadyUsed)
    {
        return false;
    }

    ExistingPreset->Name = TrimmedNewName;
    SaveConfig();
    return true;
}

bool UEasyThumbnailGeneratorUserSettings::DeletePreset(const FString& Name)
{
    const int32 RemovedCount = SavedPresets.RemoveAll(
        [&Name](const FEasyThumbnailGeneratorSavedPreset& Preset)
        {
            return Preset.Name.Equals(Name, ESearchCase::IgnoreCase);
        });

    if (RemovedCount > 0)
    {
        SaveConfig();
        return true;
    }

    return false;
}
