#include "upscaler.h"
#include <stdexcept>
#include "InteropAxodoxCommon.hpp"
#include <QPainter>
#include <stdexcept>
#include <QImage>

using namespace QtAxInterop;

void Upscaler::destroy()
{
	if (model)
		model.reset();

	Loaded = false;
}

QList<QImage> Upscaler::splitImg(const QImage& image, int chunkSize, int overlap)
{
	QList<QImage> chunks;

	int width = image.width();
	int height = image.height();

	for (int y = 0; y < height; y += chunkSize)
	{
		for (int x = 0; x < width; x += chunkSize)
		{
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

QImage Upscaler::removeOverlap(const QImage& chunk, const QSize& finalChunkSize, const QSize& originalSize, int overlap, int xPos, int yPos)
{
	bool isLeftEdge = xPos == 0;
	bool isTopEdge = yPos == 0;
	bool isRightEdge = (xPos + finalChunkSize.width() + overlap) >= originalSize.width();
	bool isBottomEdge = (yPos + finalChunkSize.height() + overlap) >= originalSize.height();

	int newWidth = isRightEdge ? (originalSize.width() - xPos) : finalChunkSize.width();
	int newHeight = isBottomEdge ? (originalSize.height() - yPos) : finalChunkSize.height();

	int startX = isLeftEdge ? 0 : overlap;
	int startY = isTopEdge ? 0 : overlap;

	QImage trimmedChunk = chunk.copy(startX, startY, newWidth, newHeight);

	return trimmedChunk;
}

QImage Upscaler::joinImgChunks(const QList<QImage>& chunks, const QSize& finalChunkSize, const QSize& originalSize, int overlap)
{
	if (chunks.isEmpty() || finalChunkSize.isEmpty() || originalSize.isEmpty())
	{
		return QImage();
	}

	QImage finalImg(originalSize, QImage::Format_RGBA8888);
	finalImg.fill(Qt::transparent);

	QPainter painter(&finalImg);

	int xPos = 0;
	int yPos = 0;

	for (int i = 0; i < chunks.count(); i++)
	{
		QImage chunk = chunks[i];
		QImage trimmedChunk = removeOverlap(chunk, finalChunkSize, originalSize, overlap, xPos, yPos);

		painter.drawImage(xPos, yPos, trimmedChunk);

		xPos += finalChunkSize.width();

		if (xPos >= originalSize.width())
		{
			xPos = 0;
			yPos += finalChunkSize.height();
		}
	}

	painter.end();
	return finalImg;
}

Upscaler::Upscaler()
{
	Loaded = false;
}

void Upscaler::load(const std::string& modelPath)
{
	destroy();

	model = std::make_unique<ESRGAN>(*env, modelPath);
	Loaded = true;
}

QImage Upscaler::upscaleImg(const QImage& inImg, uint32_t tileSize, uint32_t overlap, Axodox::Threading::async_operation_source* async_src)
{
	if (overlap >= tileSize + 1)
	{
		throw std::invalid_argument("Overlap must be less than tile size");
	}

	if (inImg.isNull())
	{
		throw std::invalid_argument("Input image is null");
	}

	uint32_t overLapDimAdd = overlap * 2;
	uint32_t realTileDim = tileSize - overLapDimAdd;
	QSize szTileSize(realTileDim, realTileDim);

	// No need for padding
	auto imgList = splitImg(inImg, tileSize, overlap);

	QList<QImage> upscaledChunks;
	QImage outChunk;

	for (auto& img : imgList)
	{
		Axodox::Graphics::TextureData texDat;
		if (img.isNull())
		{
			continue;
		}

		try
		{
			InterOpHelpers::QImageToTextureData(img, texDat);
			texDat = texDat.ToFormat(DXGI_FORMAT_B8G8R8X8_UNORM_SRGB);

			Axodox::MachineLearning::Tensor inChunk = Tensor::FromTextureData(TexDat, ColorNormalization::LinearZeroToOne);
			auto res = model->upscale(inChunk);

			auto outTexDatS = res.ToTextureData(ColorNormalization::LinearZeroToOne);

			InterOpHelper::TextureDataToQImage(outTexDatS[0], outChunk);
			upscaledChunks.push_back(outChunk.copy());

			if (async_src)
			{
				async_src->update_state((float)upscaledChunks.size() / (float)imgList.size());
			}
		}
		catch (std::exception& e)
		{
			return QImage();
		}
	}
	int32_t upscaledFactor = upscaledChunks[2].size().width() / imgList[2].size().width();

	QSize finalChunkSize = szTileSize * upscaledFactor;
	QSize finalSize = inImg.size() * upscaledFactor;
	int32_t finalOverLapSize = overlap * upscaledFactor;
	QImage finalImg = joinImgChunks(upscaledChunks, finalChunkSize, finalSize, finalOverLapSize);

	return finalImg.copy();
}
