#include "upscaler.h"
#include <stdexcept>
//#include "InteropAxodoxCommon.hpp"
#include <QPainter>

void Upscaler::destroy()
{
}

QList<QImage> Upscaler::splitImg(const QImage& image, int chunkSize, int overlap)
{
	return QList<QImage>();
}

QImage Upscaler::removeOverlap(const QImage& chunk, const QSize& finalChunkSize, const QSize& originalSize, int overlap, int xPos, int yPos)
{
	return QImage();
}

QImage Upscaler::joinImgChunks(const QList<QImage>& chunks, const QSize& finalChunkSize, const QSize& originalSize, int overlap)
{
	return QImage();
}

Upscaler::Upscaler()
{
}

void Upscaler::load(const std::string& modelPath)
{
}

QImage Upscaler::upscaleImg(const QImage& inImg, uint32_t tileSize, uint32_t overlap, Axodox::Threading::async_operation_source* async_src)
{
	return QImage();
}
