#include "WebPTestWidget.h" // Your .h file
#include "ImageUtils.h"     // For FImageUtils::CreateTexture2D (Alternative to manual UTexture2D creation)
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include "Misc/FileHelper.h"
#include "Engine/Texture2D.h"
#include "AssetRegistry/AssetRegistryModule.h" // For saving asset (optional)
// #include "EditorAssetLibrary.h" // For UEditorAssetLibrary (optional, requires EditorScriptingUtilities)

// IMPORTANT: Include your WebP Image Wrapper header.
// The path depends on where WebpImageWrapper.h is located relative to this file
// and your module's include paths.
// Assuming WebPImageSupport is a plugin and its Public folder is in include paths:
#include "Components/Image.h"
#include "WebPImageSupport/Private/WebpImageWrapper.h" // Adjust if necessary

// If WebPImageWrapper.h is in a Private folder of the same module as this test widget, it might be:
// #include "../Private/WebPImageWrapper.h"


void UWebPTestWidget::PerformWebPTest()
{
    UE_LOG(LogTemp, Log, TEXT("PerformWebPTest called!"));

    // --- 1. Instantiate your Wrapper ---
    // For this direct test, we can instantiate it directly.
    // Later, for full editor integration, the IImageWrapperModule would be used.
    TSharedPtr<FWebpImageWrapper> WebPWrapper = MakeShared<FWebpImageWrapper>();
    if (!WebPWrapper.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create FWebpImageWrapper instance."));
        return;
    }

    // --- 2. Load .webp file from disk ---
    TArray<uint8> CompressedFileData;
    // IMPORTANT: Place a test.webp file in YourProject/Content/TestImages/
    // Or adjust the path accordingly.
    FString TestWebPPath = FPaths::ProjectContentDir() / TEXT("TestImages/test.webp");
    UE_LOG(LogTemp, Log, TEXT("Attempting to load WebP from: %s"), *TestWebPPath);

    if (!FFileHelper::LoadFileToArray(CompressedFileData, *TestWebPPath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load .webp file from disk: %s"), *TestWebPPath);
        return;
    }
    UE_LOG(LogTemp, Log, TEXT("Loaded %d bytes from .webp file."), CompressedFileData.Num());

    // --- 3. Set Compressed Data ---
    if (!WebPWrapper->SetCompressed(CompressedFileData.GetData(), CompressedFileData.Num()))
    {
        UE_LOG(LogTemp, Error, TEXT("WebPWrapper->SetCompressed Failed."));
        return;
    }
    UE_LOG(LogTemp, Log, TEXT("SetCompressed Succeeded. Detected Width: %lld, Height: %lld"), WebPWrapper->GetWidth(), WebPWrapper->GetHeight());

    // --- 4. Get Raw Pixel Data (as BGRA8, common for UTexture2D) ---
    TArray64<uint8> RawPixelData;
    if (!WebPWrapper->GetRaw(ERGBFormat::BGRA, 8, RawPixelData))
    {
        UE_LOG(LogTemp, Error, TEXT("WebPWrapper->GetRaw Failed."));
        return;
    }
    UE_LOG(LogTemp, Log, TEXT("GetRaw Succeeded. Raw data size: %lld bytes."), RawPixelData.Num());

    if (WebPWrapper->GetWidth() <= 0 || WebPWrapper->GetHeight() <= 0 || RawPixelData.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Image dimensions or raw data are invalid after GetRaw."));
        return;
    }

    // --- 5. Create a UTexture2D from the Raw Data ---
    // Option A: Manual UTexture2D creation (more control, what we discussed)
    UTexture2D* NewTexture = UTexture2D::CreateTransient(WebPWrapper->GetWidth(), WebPWrapper->GetHeight(), PF_B8G8R8A8); // PF_B8G8R8A8 for BGRA
    if (!NewTexture)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create transient UTexture2D."));
        return;
    }

    // Lock the texture for writing
    void* TextureData = NewTexture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
    if (!TextureData)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to lock texture data."));
        // NewTexture->MarkAsGarbage(); // Clean up if lock fails
        return;
    }

    FMemory::Memcpy(TextureData, RawPixelData.GetData(), RawPixelData.Num());

    NewTexture->GetPlatformData()->Mips[0].BulkData.Unlock();
    NewTexture->UpdateResource(); // IMPORTANT: This uploads the data to the GPU

    UE_LOG(LogTemp, Log, TEXT("Successfully created and updated UTexture2D from WebP raw data!"));

    // --- 6. Display the Texture in the UMG Image Widget ---
    if (DisplayedImage) // Check if the UMG widget was bound
    {
        DisplayedImage->SetBrushFromTexture(NewTexture);
        // Optional: Adjust image size in UMG if the texture is very different from placeholder
        // DisplayedImage->SetDesiredSizeOverride(FVector2D(NewTexture->GetSurfaceWidth(), NewTexture->GetSurfaceHeight()));
        UE_LOG(LogTemp, Log, TEXT("SetBrushFromTexture called on DisplayedImage."));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("DisplayedImage UMG widget is not bound in C++. Ensure 'Is Variable' is checked and name matches in Blueprint."));
    }

    // --- Optional: Save as Asset (for more persistent testing) ---
    /*
    FString PackageName = TEXT("/Game/TestWebPOutput/MyGeneratedWebPTexture");
    UPackage* Package = CreatePackage(*PackageName);
    Package->FullyLoad();

    NewTexture->Rename(TEXT("MyGeneratedWebPTexture_Tex"), Package); // Give it a unique name within the package
    NewTexture->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(NewTexture);

    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = EObjectFlags::RF_Public | EObjectFlags::RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError; // Add other flags as needed
    FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());

    if (UPackage::SavePackage(Package, NewTexture, *PackageFileName, SaveArgs))
    {
        UE_LOG(LogTemp, Log, TEXT("Successfully saved texture to %s"), *PackageFileName);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save texture to %s"), *PackageFileName);
    }
    */
}

/*
// If you want to bind button click from C++
void UWebPTestWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // if (TestButton)
    // {
    //     TestButton->OnClicked.AddDynamic(this, &UWebPTestWidget::PerformWebPTest);
    // }
}
*/