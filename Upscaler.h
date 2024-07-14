#ifndef UPSCALER_H
#define UPSCALER_H
#include "ESRGAN.h"
#include <QImage>

class Upscaler
{
private:
    Axodox::MachineLearning::OnnxEnvironment* Env;
    std::unique_ptr<Axodox::MachineLearning::ESRGAN> Model;
    bool Loaded;

    void Destroy();
    QList<QImage> SplitImageIntoChunks(const QImage& image, int chunkSize = 128, int overlap = 48);
    QImage RemoveOverlapFromChunk(const QImage& chunk, const QSize& finalChunkSize, const QSize& originalSize, int overlap, int xPosition, int yPosition);
    QImage JoinImageChunks(const QList<QImage>& chunks, const QSize& finalChunkSize, const QSize& originalSize, int overlap);
public:
    Upscaler();

    void Load(const std::string& ModelPath);
    QImage UpscaleImg(const QImage& InImg, uint32_t TileSize, uint32_t Overlap = 48, Axodox::Threading::async_operation_source* async_src = nullptr);

    bool IsLoaded() { return Loaded; }
    void SetEnv(Axodox::MachineLearning::OnnxEnvironment* InEnv) { Env = InEnv; }
};

#endif 