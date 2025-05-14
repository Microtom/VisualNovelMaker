// WebpImageWrapper.cpp
#include "WebpImageWrapper.h"
#include "webp/decode.h" // Include libwebp headers
#include "webp/encode.h" // If you implement compression

// For checking the WebP signature
#include "Misc/FileHelper.h" // For FMemory::Memcmp if needed, or direct byte checks

static ERawImageFormat::Type ToRawImageFormat(ERGBFormat InRGBFormat, int32 InBitDepth)
{
    if (InBitDepth == 8)
    {
        switch (InRGBFormat)
        {
        case ERGBFormat::RGBA:
            return ERawImageFormat::BGRA8;
        case ERGBFormat::BGRA:
            return ERawImageFormat::BGRA8;
        case ERGBFormat::Gray:
            return ERawImageFormat::G8;
        default:
            // Will fall through to the final 'return ERawImageFormat::Invalid;'
                break;
        }
    }
    else if (InBitDepth == 16) // Assuming the problematic switch was here
    {
        // No specific ERGBFormat to ERawImageFormat mapping for 16-bit implemented yet.
        // The 'switch (InRGBFormat)' block has been removed.
        // If this path is taken, we'll fall through.
    }
    else if (InBitDepth == 32) // Float formats
    {
        switch (InRGBFormat)
        {
        case ERGBFormat::RGBAF:
            return ERawImageFormat::RGBA32F;
        default:
            // Will fall through
                break;
        }
    }
    // Add other 'else if (InBitDepth == X)' blocks as needed

    // Fallback for any unhandled combination or if a switch default was hit and broke
    return ERawImageFormat::Invalid;
}

FWebpImageWrapper::FWebpImageWrapper()
    : Width(0)
    , Height(0)
    , RawFormat(ERGBFormat::Invalid)
    , RawBitDepth(0)
{
}

bool FWebpImageWrapper::SetCompressed(const void* InCompressedData, int64 InCompressedSize)
{
    if (InCompressedData && InCompressedSize > 0)
    {
        // Basic WebP signature check (RIFF, size, WEBP)
        // 'R', 'I', 'F', 'F', xx, xx, xx, xx, 'W', 'E', 'B', 'P'
        const uint8* Data = static_cast<const uint8*>(InCompressedData);
        if (InCompressedSize < 12 ||
            Data[0] != 'R' || Data[1] != 'I' || Data[2] != 'F' || Data[3] != 'F' ||
            Data[8] != 'W' || Data[9] != 'E' || Data[10] != 'B' || Data[11] != 'P')
        {
            // UE_LOG(LogTemp, Warning, TEXT("Not a WebP file (magic number mismatch)."));
            return false;
        }

        CompressedData.Empty(InCompressedSize);
        CompressedData.Append(static_cast<const uint8*>(InCompressedData), InCompressedSize);
        RawData.Empty(); // Clear any previous raw data

        // Try to get info without full decode
        if (WebPGetInfo(CompressedData.GetData(), CompressedData.Num(), &Width, &Height) == 0) {
            // Failed to get info
            Width = 0;
            Height = 0;
            CompressedData.Empty();
            // UE_LOG(LogTemp, Warning, TEXT("WebPGetInfo failed."));
            return false;
        }
        return true;
    }
    return false;
}

bool FWebpImageWrapper::SetRaw(const void* InRawData, int64 InRawSize,
                               const int32 InWidth, const int32 InHeight,
                               const ERGBFormat InFormat, const int32 InBitDepth,
                               const int32 InBytesPerRow /*= 0*/) // Default value from .h doesn't need to be repeated
{
    // 1. Validate input parameters
    if (!InRawData || InRawSize <= 0 || InWidth <= 0 || InHeight <= 0)
    {
        // UE_LOG(LogTemp, Error, TEXT("FWebpImageWrapper::SetRaw: Invalid input parameters."));
        return false;
    }

    // 2. Check if the provided format and bit depth are supported for setting raw data
    //    (This usually means formats you can later compress to WebP)
    if (!CanSetRawFormat(InFormat, InBitDepth))
    {
        // UE_LOG(LogTemp, Error, TEXT("FWebpImageWrapper::SetRaw: Unsupported format (%d) or bit depth (%d) for WebP."), (int32)InFormat, InBitDepth);
        return false;
    }

    // 3. Calculate expected size based on width, height, format, and bit depth
    int32 Channels = 0;
    if (InFormat == ERGBFormat::RGBA || InFormat == ERGBFormat::BGRA)
    {
        Channels = 4;
    }
    else if (InFormat == ERGBFormat::Gray) // If you support grayscale
    {
        Channels = 1;
    }
    // Add other supported ERGBFormat cases if necessary

    if (Channels == 0)
    {
        // UE_LOG(LogTemp, Error, TEXT("FWebpImageWrapper::SetRaw: Unhandled ERGBFormat for channel calculation."));
        return false;
    }

    // Assuming 8 bits per channel for simplicity, adjust if InBitDepth can be other values (e.g., 16)
    if (InBitDepth != 8) {
        // UE_LOG(LogTemp, Error, TEXT("FWebpImageWrapper::SetRaw: Currently only supports 8-bit depth."));
        return false;
    }

    const int64 ExpectedSize = (int64)InWidth * InHeight * Channels * (InBitDepth / 8);
    if (InRawSize < ExpectedSize) // Use < because InRawSize might be larger if InBytesPerRow includes padding
    {
        // UE_LOG(LogTemp, Error, TEXT("FWebpImageWrapper::SetRaw: InRawSize (%lld) is less than expected size (%lld) for %dx%d %d-channel %d-bit image."), InRawSize, ExpectedSize, InWidth, InHeight, Channels, InBitDepth);
        return false;
    }

    // 4. Store the raw data and its properties
    Width = InWidth;
    Height = InHeight;
    RawFormat = InFormat;
    RawBitDepth = InBitDepth;

    RawData.Empty(InRawSize); // Reserve space
    // If InBytesPerRow is specified and different from Width * Channels * (InBitDepth / 8),
    // you might need to copy row by row to handle potential pitch/stride differences.
    // For simplicity, this example assumes a tightly packed buffer if InBytesPerRow is 0 or matches.
    // A more robust implementation would handle InBytesPerRow correctly.
    const int32 BytesPerPixel = Channels * (InBitDepth / 8);
    const int32 SourceRowStride = (InBytesPerRow > 0) ? InBytesPerRow : (InWidth * BytesPerPixel);
    const int32 DestRowStride = InWidth * BytesPerPixel;

    if (SourceRowStride == DestRowStride && InRawSize >= ExpectedSize)
    {
        // Data is contiguous or matches our expected tight packing
        RawData.Append(static_cast<const uint8*>(InRawData), ExpectedSize); // Copy only the expected amount
    }
    else if (InRawSize >= (int64)InHeight * SourceRowStride) // Check if InRawSize is sufficient for row-by-row copy
    {
        // UE_LOG(LogTemp, Log, TEXT("FWebpImageWrapper::SetRaw: Performing row-by-row copy due to stride mismatch or padding. SourceStride: %d, DestStride: %d"), SourceRowStride, DestRowStride);
        RawData.SetNumUninitialized(ExpectedSize); // Allocate for tightly packed data
        const uint8* SrcRow = static_cast<const uint8*>(InRawData);
        uint8* DestRow = RawData.GetData();
        for (int32 Row = 0; Row < InHeight; ++Row)
        {
            FMemory::Memcpy(DestRow, SrcRow, DestRowStride); // Copy one row (DestRowStride will be Width * BytesPerPixel)
            SrcRow += SourceRowStride;
            DestRow += DestRowStride;
        }
    }
    else
    {
        // UE_LOG(LogTemp, Error, TEXT("FWebpImageWrapper::SetRaw: InRawSize (%lld) is insufficient for row-by-row copy with SourceRowStride (%d) for %d rows."), InRawSize, SourceRowStride, InHeight);
        RawData.Empty(); // Clear any partial allocation
        Width = 0;
        Height = 0;
        RawFormat = ERGBFormat::Invalid;
        RawBitDepth = 0;
        return false;
    }


    CompressedData.Empty(); // Clear any old compressed data, as we now have new raw data

    // UE_LOG(LogTemp, Log, TEXT("FWebpImageWrapper::SetRaw: Successfully set raw data %dx%d, Format: %d, BitDepth: %d"), Width, Height, (int32)RawFormat, RawBitDepth);
    return true;
}

bool FWebpImageWrapper::PerformUncompression(const ERGBFormat InFormat, int32 InBitDepth) // Return type changed to bool
{
    if (CompressedData.Num() == 0 || Width == 0 || Height == 0)
    {
        // UE_LOG(LogTemp, Warning, TEXT("No compressed WebP data to uncompress or dimensions are zero."));
        return false; // Indicate failure
    }

    RawData.Empty();
    uint8* OutputBuffer = nullptr;
    int Stride = 0;

    // Use DecodedFormat and DecodedBitDepth (member variables from .h) instead of RawFormat/RawBitDepth locally for this function
    // The goal is to populate RawData and set DecodedFormat/DecodedBitDepth

    if (InFormat == ERGBFormat::RGBA && InBitDepth == 8)
    {
        RawFormat = ERGBFormat::RGBA; // Use the member variable declared in .h
        RawBitDepth = 8;             // Use the member variable declared in .h
        Stride = Width * 4;
        RawData.SetNumUninitialized(Stride * Height);
        OutputBuffer = WebPDecodeRGBAInto(CompressedData.GetData(), CompressedData.Num(), RawData.GetData(), RawData.Num(), Stride);
    }
    else if (InFormat == ERGBFormat::BGRA && InBitDepth == 8)
    {
        RawFormat = ERGBFormat::BGRA; // Use the member variable declared in .h
        RawBitDepth = 8;             // Use the member variable declared in .h
        Stride = Width * 4;
        RawData.SetNumUninitialized(Stride * Height);
        OutputBuffer = WebPDecodeBGRAInto(CompressedData.GetData(), CompressedData.Num(), RawData.GetData(), RawData.Num(), Stride);
    }
    // Add more formats as needed

    if (OutputBuffer == nullptr)
    {
        // UE_LOG(LogTemp, Error, TEXT("WebPDecode failed."));
        RawData.Empty();
        RawFormat = ERGBFormat::Invalid; // Reset decoded format on failure
        RawBitDepth = 0;                // Reset decoded bit depth on failure
        // Width = 0; // You might not want to reset Width/Height here, as they come from WebPGetInfo
        // Height = 0;
        return false; // Indicate failure
    }
    return true; // Indicate success
}

bool FWebpImageWrapper::GetRaw(const ERGBFormat InFormat, int32 InBitDepth, TArray64<uint8>& OutRawData)
{
    if (RawData.Num() == 0 || RawFormat == ERGBFormat::Invalid) // If not yet uncompressed or failed
    {
        PerformUncompression(InFormat, InBitDepth); // Try to uncompress with the requested format
    }

    if (RawData.Num() > 0 && RawFormat == InFormat && RawBitDepth == InBitDepth)
    {
        OutRawData = RawData;
        return true;
    }
    // TODO: Handle format/bitdepth conversion if RawData is in a different format than requested
    // UE_LOG(LogTemp, Warning, TEXT("WebP GetRaw: Format mismatch or uncompression failed."));
    return false;
}

bool FWebpImageWrapper::GetRaw(const ERGBFormat InRequestedRGBFormat, int32 InRequestedBitDepth, FDecompressedImageOutput& OutDecompressedImage)
{
    TArray64<uint8> TempRawPixelData;
    // This call to GetRaw should ensure that this->RawData, this->RawFormat, this->RawBitDepth,
    // this->Width, and this->Height are populated correctly with the uncompressed image data
    // in the InRequestedRGBFormat and InRequestedBitDepth.
    if (!GetRaw(InRequestedRGBFormat, InRequestedBitDepth, TempRawPixelData))
    {
        // UE_LOG(LogTemp, Warning, TEXT("FWebpImageWrapper::GetRaw(FDecompressedImageOutput): Failed to get raw pixel data via TArray overload."));
        return false;
    }

    // At this point, TempRawPixelData contains the image data in InRequestedRGBFormat.
    // this->Width and this->Height should be the dimensions of this image.
    // this->RawFormat and this->RawBitDepth should match InRequestedRGBFormat and InRequestedBitDepth.

    if (this->RawData.Num() == 0 || this->RawFormat != InRequestedRGBFormat || this->RawBitDepth != InRequestedBitDepth || this->Width <= 0 || this->Height <= 0)
    {
        // This is a sanity check; if the above GetRaw succeeded, this state should be consistent.
        // UE_LOG(LogTemp, Error, TEXT("FWebpImageWrapper::GetRaw(FDecompressedImageOutput): Inconsistent internal state after getting raw pixel data."));
        return false;
    }

    ERawImageFormat::Type TargetRawImageFormat = ToRawImageFormat(InRequestedRGBFormat, InRequestedBitDepth);
    if (TargetRawImageFormat == ERawImageFormat::Invalid)
    {
        // UE_LOG(LogTemp, Error, TEXT("FWebpImageWrapper::GetRaw(FDecompressedImageOutput): Could not convert ERGBFormat (%d) to ERawImageFormat."), (int32)InRequestedRGBFormat);
        return false;
    }

    // Initialize FMipMapImage (assuming sRGB for typical WebP, adjust if needed)
    // The Init function will clear SubImages.
    OutDecompressedImage.MipMapImage.Init(this->Width, this->Height, 1, TargetRawImageFormat, EGammaSpace::sRGB);

    // Since WebP typically represents a single image (mip 0), we add one mip.
    // The FMipMapImage::Init above already prepared for one mip (MipZeroWidth, MipZeroHeight, NumMips=1)
    // and should have created one entry in SubImages.

    if (OutDecompressedImage.MipMapImage.SubImages.Num() > 0)
    {
        // The Init function should have set up the first mip's dimensions.
        // Let's ensure they match and set the data.
        FMipMapImage::FMipInfo& MipInfo = OutDecompressedImage.MipMapImage.SubImages[0];
        MipInfo.Width = this->Width;   // Width of the base mip
        MipInfo.Height = this->Height; // Height of the base mip
        MipInfo.Offset = 0;            // Data for the first mip starts at offset 0
        MipInfo.Size = TempRawPixelData.Num(); // Size of the mip data in bytes

        OutDecompressedImage.MipMapImage.RawData = MoveTemp(TempRawPixelData); // Move the pixel data
        OutDecompressedImage.MipMapImage.Format = TargetRawImageFormat;
        // GammaSpace is already set by Init
    }
    else
    {
        // This shouldn't happen if Init worked correctly
        // UE_LOG(LogTemp, Error, TEXT("FWebpImageWrapper::GetRaw(FDecompressedImageOutput): MipMapImage.SubImages is empty after Init."));
        return false;
    }

    return true;
}

int64 FWebpImageWrapper::GetWidth() const
{
    return static_cast<int64>(this->Width); // Return the stored width
}

int64 FWebpImageWrapper::GetHeight() const
{
    return static_cast<int64>(this->Height); // Return the stored height
}

// Implement other IImageWrapper methods (GetWidth, GetHeight, GetFormat, GetBitDepth, CanSetRawFormat, Compress, etc.)
// GetFormat() should return the format of the *raw* data after uncompression.
// GetBitDepth() should return the bit depth of the *raw* data.
// Compress() would use libwebp's encoding functions like WebPEncodeRGBA().
// CanSetRawFormat needs to check if you can handle the requested format and bit depth for compression.

int32 FWebpImageWrapper::GetBitDepth() const
{
    return RawBitDepth; // Or derive from RawFormat
}

ERGBFormat FWebpImageWrapper::GetFormat() const
{
    return RawFormat; // Or derive from what libwebp outputs
}

bool FWebpImageWrapper::CanSetRawFormat(const ERGBFormat InFormat, const int32 InBitDepth) const
{
    // Check if libwebp supports encoding this format or if you can convert to a supported one
    return (InFormat == ERGBFormat::RGBA || InFormat == ERGBFormat::BGRA) && InBitDepth == 8;
}

ERawImageFormat::Type FWebpImageWrapper::GetSupportedRawFormat(const ERawImageFormat::Type InFormat) const
{
    // This function's goal is to map an incoming ERawImageFormat request
    // to a format your wrapper can actually produce.
    // Often, textures are imported and might be in various ERawImageFormat types,
    // and this function helps the engine figure out what format to request from your GetRaw.

    // If the requested format is BGRA8 and you can produce ERGBFormat::BGRA at 8-bit, that's a direct match.
    if (InFormat == ERawImageFormat::BGRA8 && CanSetRawFormat(ERGBFormat::BGRA, 8))
    {
        return ERawImageFormat::BGRA8;
    }

    // If the requested format is G8 and you can produce ERGBFormat::Gray at 8-bit
    if (InFormat == ERawImageFormat::G8 && CanSetRawFormat(ERGBFormat::Gray, 8)) // Assuming you add Gray support
    {
        return ERawImageFormat::G8;
    }

    // If the engine requests something else, but you primarily work with BGRA or RGBA,
    // suggest BGRA8 as the preferred output format if you support it.
    // The engine might then do its own conversions if needed.
    if (CanSetRawFormat(ERGBFormat::BGRA, 8))
    {
        // UE_LOG(LogTemp, Verbose, TEXT("FWebpImageWrapper::GetSupportedRawFormat: Input format %d not directly supported or preferred, suggesting BGRA8."), (int32)InFormat);
        return ERawImageFormat::BGRA8;
    }

    // If you also robustly support outputting ERGBFormat::RGBA, and for some reason BGRA is not available
    // (though for WebP, BGRA is usually fine), you might have a fallback.
    // However, ERawImageFormat doesn't have a direct RGBA8. The closest is BGRA8,
    // and the engine handles the R/B swap if it needs actual RGBA pixel order for something specific.
    // So, sticking to BGRA8 as the primary suggestion is best.

    // If for some reason your wrapper CANNOT produce BGRA8 (e.g., libwebp only gave you RGBA and you didn't convert)
    // then this function becomes more complex, as you'd have to indicate what you *can* produce.
    // But usually, you'd aim to provide data in ERawImageFormat::BGRA8.

    // Fallback: if none of the preferred formats are supported by your wrapper, indicate invalid.
    // This should ideally not be hit if your wrapper is functional for common cases.
    // UE_LOG(LogTemp, Warning, TEXT("FWebpImageWrapper::GetSupportedRawFormat: No supported output format found for input %d. Wrapper might be misconfigured or input format too exotic."), (int32)InFormat);
    return ERawImageFormat::Invalid;
}


TArray64<uint8> FWebpImageWrapper::GetCompressed(int32 Quality)
{
    if (RawData.Num() == 0 || Width == 0 || Height == 0 || RawFormat == ERGBFormat::Invalid)
    {
        // UE_LOG(LogTemp, Warning, TEXT("No raw data to compress for WebP. Returning empty array."));
        return TArray64<uint8>();
    }

    TArray64<uint8> OutCompressedData;
    uint8_t* webp_output_buffer = nullptr;
    size_t webp_output_size = 0;
    int stride = 0;

    if (RawFormat == ERGBFormat::RGBA && RawBitDepth == 8)
    {
        stride = Width * 4;
        webp_output_size = WebPEncodeRGBA(RawData.GetData(), Width, Height, stride, (float)Quality, &webp_output_buffer);
    }
    else if (RawFormat == ERGBFormat::BGRA && RawBitDepth == 8)
    {
        stride = Width * 4;
        webp_output_size = WebPEncodeBGRA(RawData.GetData(), Width, Height, stride, (float)Quality, &webp_output_buffer);
    }
    // No ERGBFormat::RGB case here because it's not in the standard enum
    else
    {
        // UE_LOG(LogTemp, Warning, TEXT("WebP Compression: Unsupported RawFormat (%d) or BitDepth (%d). Returning empty array."), (int32)RawFormat, RawBitDepth);
        return TArray64<uint8>();
    }

    if (webp_output_size > 0 && webp_output_buffer != nullptr)
    {
        OutCompressedData.Append(webp_output_buffer, webp_output_size);
        WebPFree(webp_output_buffer);
        return OutCompressedData;
    }
    else
    {
        // UE_LOG(LogTemp, Error, TEXT("WebPEncode failed for format %d. Returning empty array."), (int32)RawFormat);
        return TArray64<uint8>();
    }
}
