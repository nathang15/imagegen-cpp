#ifndef UPSCALER_H
#define UPSCALER_H
#include "ESRGAN.h"
#include <QImage>

class Upscaler
{
private:
    Axodox::MachineLearning::OnnxEnvironment* env;
    std::unique_ptr<Axodox::MachineLearning::ESRGAN> model;
    bool loaded;

    void destroy();
    QList<QImage> splitImg(
        const QImage& image,
        int chunkSize = 128, 
        int overlap = 48
    );
    QImage removeOverlap(
        const QImage& chunk, 
        const QSize& finalChunkSize, 
        const QSize& originalSize, 
        int overlap, 
        int xPos, 
        int yPos);
    QImage joinImgChunks(
        const QList<QImage>& chunks, 
        const QSize& finalChunkSize, 
        const QSize& originalSize, 
        int overlap);

public:
    Upscaler();

    void load(const std::string& modelPath);
    QImage upscaleImg(
        const QImage& inImg, 
        uint32_t tileSize, 
        uint32_t overlap = 48, 
        Axodox::Threading::async_operation_source* async_src = nullptr);

    bool loaded() { return Loaded; }
    void setEnv(Axodox::MachineLearning::OnnxEnvironment* inEnv) { env = inEnv; }
};

#endif