#include "Upscaler.h"

using namespace Axodox::Graphics;
using namespace Axodox::MachineLearning;
using namespace Axodox::Collections;
#include "InteropAxodoxCommon.hpp"

using namespace QtAxInterop;
#include <stdexcept>

#include <QPainter>

QList<QImage> Upscaler::SplitImageIntoChunks(const QImage& image, int chunkSize, int overlap) {
    QList<QImage> chunks;

    int width = image.width();
    int height = image.height();

    for (int y = 0; y < height; y += chunkSize) {
        for (int x = 0; x < width; x += chunkSize) {
            int startX = x == 0 ? 0 : x - overlap;
            int startY = y == 0 ? 0 : y - overlap;

            int chunkWidth = chunkSize + (x == 0 ? 0 : overlap) + ((x + chunkSize >= width) ? 0 : overlap);
            int chunkHeight = chunkSize + (y == 0 ? 0 : overlap) + ((y + chunkSize >= height) ? 0 : overlap);

            chunkWidth = (startX + chunkWidth > width) ? width - startX : chunkWidth;
            chunkHeight = (startY + chunkHeight > height) ? height - startY : chunkHeight;

            QImage chunk = image.copy(startX, startY, chunkWidth, chunkHeight);
            chunks.append(chunk);
        }
    }

    return chunks;
}

QImage Upscaler::RemoveOverlapFromChunk(const QImage& chunk, const QSize& finalChunkSize, const QSize& originalSize, int overlap, int xPosition, int yPosition) {
    bool isLeftEdge = xPosition == 0;
    bool isTopEdge = yPosition == 0;

    bool isRightEdge = (xPosition + finalChunkSize.width() + overlap) >= originalSize.width();
    bool isBottomEdge = (yPosition + finalChunkSize.height() + overlap) >= originalSize.height();

    int newWidth = isRightEdge ? (originalSize.width() - xPosition) : finalChunkSize.width();
    int newHeight = isBottomEdge ? (originalSize.height() - yPosition) : finalChunkSize.height();

    int startX = isLeftEdge ? 0 : overlap;
    int startY = isTopEdge ? 0 : overlap;

    QImage trimmedChunk = chunk.copy(startX, startY, newWidth, newHeight);

    return trimmedChunk;
}

QImage Upscaler::JoinImageChunks(const QList<QImage>& chunks, const QSize& finalChunkSize, const QSize& originalSize, int overlap) {
    if (chunks.isEmpty() || finalChunkSize.isEmpty() || originalSize.isEmpty()) {
        return QImage();
    }

    QImage finalImage(originalSize, QImage::Format_RGBA8888);
    finalImage.fill(Qt::transparent);

    QPainter painter(&finalImage);

    int xPosition = 0;
    int yPosition = 0;

    for (int i = 0; i < chunks.count(); ++i) {
        QImage trimmedChunk = RemoveOverlapFromChunk(chunks[i], finalChunkSize, originalSize, overlap, xPosition, yPosition);

        painter.drawImage(xPosition, yPosition, trimmedChunk);

        xPosition += finalChunkSize.width();
        if (xPosition >= originalSize.width()) {
            xPosition = 0;
            yPosition += finalChunkSize.height();
        }
    }

    painter.end();
    return finalImage;
}

void Upscaler::Destroy()
{

    if (Model)
        Model.reset();

    Loaded = false;
}

Upscaler::Upscaler() {
    Loaded = false;
}

void Upscaler::Load(const std::string& ModelPath)
{
    Destroy();

    Model = std::make_unique<ESRGAN>(*Env, ModelPath);
    Loaded = true;

}

QImage Upscaler::UpscaleImg(const QImage& InImg, uint32_t TileSize, uint32_t Overlap, Axodox::Threading::async_operation_source* async_src)
{
    if (Overlap > TileSize + 1)
        throw std::invalid_argument("Overlap cannot be bigger than or equal to tile size!");

    if (InImg.isNull())
        throw std::invalid_argument("Cannot upscale a null or invalid image!");

    uint32_t OverlapDimensionsAdd = Overlap * 2;
    uint32_t RealTileDim = TileSize - OverlapDimensionsAdd;
    QSize szTileSize(RealTileDim, RealTileDim);
    auto ImgList = SplitImageIntoChunks(InImg, RealTileDim, Overlap);

    QList<QImage> UpsChunks;
    QImage OutChunk;

    for (auto& Img : ImgList)
    {
        Axodox::Graphics::TextureData TexDat;

        if (Img.isNull()) {
            continue;
        }

        try {

            InterOpHelper::QImageToTextureData(Img, TexDat);

            TexDat = TexDat.ToFormat(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB);

            Axodox::MachineLearning::Tensor InpChunk = Tensor::FromTextureData(TexDat, ColorNormalization::LinearZeroToOne);
            auto res = Model->Upscale(InpChunk);

            auto OutTexDatS = res.ToTextureData(ColorNormalization::LinearZeroToOne);



            InterOpHelper::TextureDataToQImage(OutTexDatS[0], OutChunk);
            UpsChunks.push_back(OutChunk.copy());

            if (async_src) {
                async_src->update_state((float)UpsChunks.size() / (float)ImgList.size());

            }
        }
        catch (std::exception& Ex) {
            return QImage();
        }
    }
    int32_t UpsFactor = UpsChunks[2].size().width() / ImgList[2].size().width();

    QSize FinalChunkSize = szTileSize * UpsFactor;
    QSize FinalSize = InImg.size() * UpsFactor;
    int32_t FinalOverlapSize = Overlap * UpsFactor;

    QImage FinalImg = JoinImageChunks(UpsChunks, FinalChunkSize, FinalSize, FinalOverlapSize);

    return FinalImg.copy();


}