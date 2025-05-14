// WebpImageWrapper.h
#pragma once

#include "CoreMinimal.h"
#include "IImageWrapper.h"
// Forward declare from libwebp if necessary, or include webp/decode.h here
// #include "webp/decode.h" // Example, better in .cpp if possible

class FWebpImageWrapper : public IImageWrapper
{
public:
    FWebpImageWrapper();
    virtual ~FWebpImageWrapper() override = default; // Good practice to have a virtual destructor

    //~ Begin IImageWrapper Interface
    virtual bool SetCompressed(const void* InCompressedData, int64 InCompressedSize) override;
    virtual bool SetRaw(const void* InRawData, int64 InRawSize, const int32 InWidth, const int32 InHeight, const ERGBFormat InFormat, const int32 InBitDepth, const int32 InBytesPerRow = 0) override;
    virtual bool CanSetRawFormat(const ERGBFormat InFormat, const int32 InBitDepth) const override;
    virtual ERawImageFormat::Type GetSupportedRawFormat(const ERawImageFormat::Type InFormat) const override; // New addition based on IImageWrapper
    virtual TArray64<uint8> GetCompressed(int32 Quality = (int32)EImageCompressionQuality::Default) override; // Matched signature

    // The primary GetRaw to implement (others call this or GetRawImage)
    virtual bool GetRaw(const ERGBFormat InFormat, int32 InBitDepth, TArray64<uint8>& OutRawData) override;
    // You also need to implement this one if you support metadata/mips, otherwise a basic implementation
    virtual bool GetRaw(const ERGBFormat InFormat, int32 InBitDepth, FDecompressedImageOutput& OutDecompressedImage) override;


    virtual int64 GetWidth() const override;
    virtual int64 GetHeight() const override;
    virtual int32 GetBitDepth() const override;
    virtual ERGBFormat GetFormat() const override;
    //~ End IImageWrapper Interface

    // Helper to get compressed size (not an override)
    int64 GetSizeOfCompressedData() const { return CompressedData.Num(); }


private:
    // Internal helper for decompression logic, not virtual, not an override
    bool PerformUncompression(const ERGBFormat InFormat, int32 InBitDepth);

    TArray64<uint8> CompressedData;
    TArray64<uint8> RawData; // Stores the uncompressed pixel data

    int32 Width;
    int32 Height;
    ERGBFormat RawFormat; // The format of the data in RawData after decoding
    int32 RawBitDepth;    // The bit depth of the data in RawData after decoding
};