using UnrealBuildTool;

public class EasyThumbnailGenerator : ModuleRules
{
    public EasyThumbnailGenerator(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "InputCore"
        });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "AssetRegistry",
            "ContentBrowser",
            "CoreUObject",
            "Engine",
            "ImageCore",
            "PropertyEditor",
            "RenderCore",
            "Slate",
            "SlateCore",
            "ToolMenus",
            "UnrealEd"
        });
    }
}
