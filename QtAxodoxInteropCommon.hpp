#pragma once
#include "Include/Axodox.Graphics.h"
#include "Include/Axodox.Collections.h"
#include "Include/Axodox.Infrastructure.h"
#include "Include/Axodox.MachineLearning.h"

#include <QImage>

namespace QtAxInterop {
    class InterOpHelper
    {
        InterOpHelper() {};

    public:
        static void QImageToTextureData(QImage& ReadyImage, Axodox::Graphics::TextureData& TexDat)
        {
            ReadyImage = ReadyImage.convertToFormat(QImage::Format_RGBA8888);
            TexDat.Width = ReadyImage.width(); TexDat.Height = ReadyImage.height();
            TexDat.Stride = ReadyImage.bytesPerLine();
            TexDat.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

            const uint8_t* data = reinterpret_cast<const uint8_t*>(ReadyImage.bits());
            size_t dataSize = static_cast<size_t>(ReadyImage.sizeInBytes());

            TexDat.Buffer.assign(data, data + dataSize);

        }

        static void TextureDataToQImage(Axodox::Graphics::TextureData& TexDat, QImage& OutImg)
        {

            int bytesPerPixel = 4;
            QImage::Format format = QImage::Format_RGBA8888;

            auto ImageBuffer = TexDat.ToFormat(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB).Buffer;

            QImage image(ImageBuffer.data(), TexDat.Width, TexDat.Height, TexDat.Width * bytesPerPixel, format);

            OutImg = image.copy();
        }
    };
}