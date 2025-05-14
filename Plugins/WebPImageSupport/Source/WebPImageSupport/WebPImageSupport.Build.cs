// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class WebPImageSupport : ModuleRules
{
    public WebPImageSupport(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] {
                // ... add public include paths required here ...
            }
            );


        PrivateIncludePaths.AddRange(
            new string[] {
                // ... add other private include paths required here ...
            }
            );


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "ImageWrapper", // Make sure this is here
                "ImageCore",
            }
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
            }
            );


        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
                // ... add any modules that your module loads dynamically here ...
            }
            );

        // ModuleDirectory is .../Plugins/WebPImageSupport/Source/WebPImageSupport/
        // Go up one level to .../Plugins/WebPImageSupport/Source/
        string SourceDir = Path.GetFullPath(Path.Combine(ModuleDirectory, ".."));
        string LibWebPBaseDir = Path.Combine(SourceDir, "ThirdParty", "libwebp");

        System.Console.WriteLine("WebPImageSupport: LibWebPBaseDir = " + LibWebPBaseDir);
        if (!Directory.Exists(LibWebPBaseDir))
        {
            System.Console.WriteLine("WebPImageSupport: ERROR - LibWebPBaseDir does not exist: " + LibWebPBaseDir);
        }

        // ****** THIS IS THE KEY CHANGE ******
        // The 'IncludePath' should be the directory that *contains* the 'webp' folder.
        // In your structure, 'webp' is directly inside 'LibWebPBaseDir'.
        // So, the include path is just LibWebPBaseDir itself.
        string IncludePath = LibWebPBaseDir;
        PublicIncludePaths.Add(IncludePath);
        System.Console.WriteLine("WebPImageSupport: Adding Include Path: " + IncludePath);

        // Check if the 'webp' subdirectory actually exists within the new IncludePath
        string WebpSpecificHeaderDir = Path.Combine(IncludePath, "webp");
        if (!Directory.Exists(WebpSpecificHeaderDir))
        {
            System.Console.WriteLine("WebPImageSupport: WARNING - 'webp' subdirectory not found at: " + WebpSpecificHeaderDir);
            System.Console.WriteLine("WebPImageSupport: Make sure decode.h is at: " + Path.Combine(WebpSpecificHeaderDir, "decode.h"));
        }


        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            // Path to the static library file
            string LibFilePath = Path.Combine(LibWebPBaseDir, "lib", "libwebp.lib");
            PublicAdditionalLibraries.Add(LibFilePath);
            System.Console.WriteLine("WebPImageSupport: Looking for libwebp.lib at: " + LibFilePath);

            if (!File.Exists(LibFilePath))
            {
                System.Console.WriteLine("WebPImageSupport: ERROR - libwebp.lib not found at: " + LibFilePath);
            }
            
        }
    }
}