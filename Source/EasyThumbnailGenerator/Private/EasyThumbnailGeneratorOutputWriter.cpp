#include "EasyThumbnailGeneratorOutputWriter.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Texture2D.h"
#include "HAL/FileManager.h"
#include "ImageCore.h"
#include "ImageUtils.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

#define LOCTEXT_NAMESPACE "EasyThumbnailGeneratorOutputWriter"

namespace EasyThumbnailGenerator
{
    namespace
    {
        bool UsesPNG(EEasyThumbnailGeneratorOutputMode Mode)
        {
            return Mode != EEasyThumbnailGeneratorOutputMode::TextureAsset;
        }

        bool UsesTexture(EEasyThumbnailGeneratorOutputMode Mode)
        {
            return Mode != EEasyThumbnailGeneratorOutputMode::PNG;
        }

        bool BuildBaseName(
            const FString& AssetName,
            const FEasyThumbnailGeneratorRenderSettings& Settings,
            FString& OutName,
            FText& OutError)
        {
            FString BaseName = AssetName;
            const FString SourcePrefix = Settings.SourcePrefixToStrip.TrimStartAndEnd();
            if (!SourcePrefix.IsEmpty() && BaseName.StartsWith(SourcePrefix, ESearchCase::CaseSensitive))
            {
                BaseName.RightChopInline(SourcePrefix.Len(), EAllowShrinking::No);
            }

            FString Template = Settings.FilenameTemplate.TrimStartAndEnd();
            if (Template.IsEmpty())
            {
                Template = TEXT("{Prefix}{BaseName}");
            }

            OutName = Template.Replace(TEXT("{AssetName}"), *AssetName, ESearchCase::CaseSensitive);
            OutName = OutName.Replace(TEXT("{BaseName}"), *BaseName, ESearchCase::CaseSensitive);
            OutName = OutName.Replace(TEXT("{Prefix}"), *Settings.OutputPrefix, ESearchCase::CaseSensitive);

            if (OutName.IsEmpty() || OutName == TEXT(".") || OutName == TEXT("..") ||
                OutName.Contains(TEXT("{")) || OutName.Contains(TEXT("}")) ||
                FPaths::MakeValidFileName(OutName) != OutName)
            {
                OutError = LOCTEXT(
                    "InvalidOutputName",
                    "The filename template produced an invalid name. Use {Prefix}, {BaseName} or {AssetName} and avoid path separators or reserved characters.");
                return false;
            }

            return true;
        }

        bool PrepareDestinations(
            const FEasyThumbnailGeneratorRenderSettings& Settings,
            FString& OutPNGDirectory,
            FString& OutTextureDirectory,
            FText& OutError)
        {
            if (UsesPNG(Settings.OutputMode))
            {
                const FString ConfiguredDirectory = Settings.PNGOutputDirectory.TrimStartAndEnd().IsEmpty()
                    ? TEXT("Saved/Thumbnails")
                    : Settings.PNGOutputDirectory.TrimStartAndEnd();

                OutPNGDirectory = FPaths::IsRelative(ConfiguredDirectory)
                    ? FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), ConfiguredDirectory)
                    : ConfiguredDirectory;

                FPaths::NormalizeDirectoryName(OutPNGDirectory);
                if (OutPNGDirectory.IsEmpty())
                {
                    OutError = LOCTEXT("InvalidPNGDirectory", "The PNG output directory is empty.");
                    return false;
                }
            }

            if (UsesTexture(Settings.OutputMode))
            {
                OutTextureDirectory = Settings.TextureAssetDirectory.TrimStartAndEnd();
                OutTextureDirectory.RemoveFromEnd(TEXT("/"));

                if (OutTextureDirectory != TEXT("/Game") &&
                    !OutTextureDirectory.StartsWith(TEXT("/Game/")))
                {
                    OutError = LOCTEXT(
                        "InvalidTextureDirectory",
                        "Texture Asset Directory must be under /Game, for example /Game/UI/Thumbnails.");
                    return false;
                }
            }

            return true;
        }

        FString BuildTexturePackageName(const FString& Directory, const FString& Name)
        {
            return Directory / Name;
        }

        bool IsTexturePackageOccupied(const FString& PackageName)
        {
            if (FPackageName::DoesPackageExist(PackageName, nullptr, false))
            {
                return true;
            }

            // Also detect newly created but not-yet-saved packages in this editor session.
            return FindPackage(nullptr, *PackageName) != nullptr;
        }

        bool SavePNG(
            const FString& FilePath,
            const FEasyThumbnailGeneratorRenderSettings& Settings,
            const TArray<FColor>& Pixels,
            FText& OutError)
        {
            const FString Directory = FPaths::GetPath(FilePath);
            if (!IFileManager::Get().MakeDirectory(*Directory, true) &&
                !IFileManager::Get().DirectoryExists(*Directory))
            {
                OutError = FText::Format(
                    LOCTEXT("CreatePNGDirectoryFailed", "Could not create output directory: {0}"),
                    FText::FromString(Directory));
                return false;
            }

            // FImageView's UE 5.8 constructor takes a mutable void* even though
            // SaveImageByExtension only reads these pixels.
            const FImageView ImageView(
                const_cast<FColor*>(Pixels.GetData()),
                Settings.OutputResolution,
                Settings.OutputResolution,
                1,
                ERawImageFormat::BGRA8,
                EGammaSpace::sRGB);

            if (!FImageUtils::SaveImageByExtension(*FilePath, ImageView, 100))
            {
                OutError = FText::Format(
                    LOCTEXT("SavePNGFailed", "Failed to save PNG thumbnail: {0}"),
                    FText::FromString(FilePath));
                return false;
            }

            return true;
        }

        bool SaveTextureAsset(
            const FString& LongPackageName,
            const FString& AssetName,
            const FEasyThumbnailGeneratorRenderSettings& Settings,
            const TArray<FColor>& Pixels,
            FText& OutError)
        {
            if (!FPackageName::IsValidLongPackageName(
                    FStringView(LongPackageName), false, static_cast<FText*>(nullptr)))
            {
                OutError = FText::Format(
                    LOCTEXT("InvalidTexturePackageName", "Invalid Texture Asset package name: {0}"),
                    FText::FromString(LongPackageName));
                return false;
            }

            const bool bPackageOnDisk =
                FPackageName::DoesPackageExist(LongPackageName, nullptr, false);
            UTexture2D* Texture = bPackageOnDisk
                ? LoadObject<UTexture2D>(
                    nullptr, *(LongPackageName + TEXT(".") + AssetName))
                : nullptr;

            if (bPackageOnDisk && !Texture)
            {
                OutError = FText::Format(
                    LOCTEXT("ExistingTextureLoadFailed", "The existing asset cannot be loaded as a Texture2D: {0}"),
                    FText::FromString(LongPackageName));
                return false;
            }

            UPackage* Package = Texture ? Texture->GetOutermost() : CreatePackage(*LongPackageName);
            if (!Package)
            {
                OutError = LOCTEXT("TexturePackageFailed", "Could not create the Texture Asset package.");
                return false;
            }

            if (!Texture)
            {
                Texture = FindObject<UTexture2D>(Package, *AssetName);
            }

            if (!Texture && FindObject<UObject>(Package, *AssetName))
            {
                OutError = FText::Format(
                    LOCTEXT("TextureNameOccupied", "An asset of another type already uses this name: {0}"),
                    FText::FromString(LongPackageName));
                return false;
            }

            const bool bNewTexture = Texture == nullptr;
            if (bNewTexture)
            {
                Texture = NewObject<UTexture2D>(
                    Package,
                    *AssetName,
                    RF_Public | RF_Standalone | RF_Transactional);
            }

            if (!Texture)
            {
                OutError = LOCTEXT("TextureCreateFailed", "Could not create the output Texture Asset.");
                return false;
            }

            Texture->Modify();
            Texture->PreEditChange(nullptr);
            Texture->Source.Init(
                Settings.OutputResolution,
                Settings.OutputResolution,
                1,
                1,
                TSF_BGRA8,
                reinterpret_cast<const uint8*>(Pixels.GetData()));
            Texture->SRGB = true;
            Texture->CompressionSettings = TC_Default;
            Texture->MipGenSettings = TMGS_NoMipmaps;
            Texture->NeverStream = true;
            Texture->PostEditChange();

            if (bNewTexture)
            {
                FAssetRegistryModule::AssetCreated(Texture);
            }

            Package->MarkPackageDirty();

            const FString Filename = FPackageName::LongPackageNameToFilename(
                LongPackageName, FPackageName::GetAssetPackageExtension());
            const FString Directory = FPaths::GetPath(Filename);

            if (!IFileManager::Get().MakeDirectory(*Directory, true) &&
                !IFileManager::Get().DirectoryExists(*Directory))
            {
                OutError = FText::Format(
                    LOCTEXT("CreateTextureDirectoryFailed", "Could not create the Texture Asset directory: {0}"),
                    FText::FromString(Directory));
                return false;
            }

            FSavePackageArgs SaveArgs;
            SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
            if (!UPackage::SavePackage(Package, Texture, *Filename, SaveArgs))
            {
                OutError = FText::Format(
                    LOCTEXT("SaveTextureFailed", "Failed to save Texture Asset: {0}"),
                    FText::FromString(LongPackageName));
                return false;
            }

            return true;
        }
    }

    bool WriteThumbnailOutputs(
        UObject* Asset,
        const FEasyThumbnailGeneratorRenderSettings& Settings,
        const TArray<FColor>& Pixels,
        FString& OutLocation,
        bool& bOutSkipped,
        FText& OutError)
    {
        OutLocation.Reset();
        bOutSkipped = false;
        OutError = FText::GetEmpty();

        if (!IsValid(Asset) || Pixels.Num() != Settings.OutputResolution * Settings.OutputResolution)
        {
            OutError = LOCTEXT("InvalidOutputPixels", "Invalid rendered thumbnail data.");
            return false;
        }

        FString Name;
        if (!BuildBaseName(Asset->GetName(), Settings, Name, OutError))
        {
            return false;
        }

        FString PNGDirectory;
        FString TextureDirectory;
        if (!PrepareDestinations(Settings, PNGDirectory, TextureDirectory, OutError))
        {
            return false;
        }

        const bool bPNG = UsesPNG(Settings.OutputMode);
        const bool bTexture = UsesTexture(Settings.OutputMode);
        FString SelectedName = Name;

        // Choose one shared suffix for Both mode so the two outputs keep the same name.
        for (int32 Suffix = 0; Suffix < 100000; ++Suffix)
        {
            SelectedName = Suffix == 0
                ? Name
                : FString::Printf(TEXT("%s_%d"), *Name, Suffix);

            const FString PNGFile = FPaths::Combine(PNGDirectory, SelectedName + TEXT(".png"));
            const FString TexturePackage = BuildTexturePackageName(TextureDirectory, SelectedName);

            if (bTexture && !FPackageName::IsValidLongPackageName(
                    FStringView(TexturePackage), false, static_cast<FText*>(nullptr)))
            {
                OutError = FText::Format(
                    LOCTEXT("InvalidOutputPackage", "Invalid output Texture Asset name: {0}"),
                    FText::FromString(TexturePackage));
                return false;
            }

            const bool bPNGExists = bPNG && IFileManager::Get().FileExists(*PNGFile);
            const bool bTextureExists = bTexture && IsTexturePackageOccupied(TexturePackage);
            const bool bCollision = bPNGExists || bTextureExists;

            if (bCollision && Settings.OverwritePolicy == EEasyThumbnailGeneratorOverwritePolicy::AutoIncrement)
            {
                continue;
            }

            if (bCollision && Settings.OverwritePolicy == EEasyThumbnailGeneratorOverwritePolicy::SkipExisting)
            {
                bOutSkipped = true;
                OutLocation = bPNG ? PNGFile : TexturePackage;
                return true;
            }

            if (bPNG && !SavePNG(PNGFile, Settings, Pixels, OutError))
            {
                return false;
            }

            if (bTexture && !SaveTextureAsset(TexturePackage, SelectedName, Settings, Pixels, OutError))
            {
                if (bPNG)
                {
                    OutError = FText::Format(
                        LOCTEXT("PartialOutputFailure", "PNG was written to {0}, but the Texture Asset failed: {1}"),
                        FText::FromString(PNGFile), OutError);
                }
                return false;
            }

            OutLocation = bPNG && bTexture
                ? FString::Printf(TEXT("%s and %s"), *PNGFile, *TexturePackage)
                : (bPNG ? PNGFile : TexturePackage);
            return true;
        }

        OutError = LOCTEXT("NoUniqueName", "Could not find an unused thumbnail filename.");
        return false;
    }
}

#undef LOCTEXT_NAMESPACE
