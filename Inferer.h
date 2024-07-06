#ifndef INFERER_H
#define INFERER_H

#include <QThread>
#include "SDModel.h"
#include "Upscaler.h"
#include <QImage>

enum class SDJobType {
    txt2Img,
    img2Img,
    upscale
};
class Inferer : public QThread
{
    Q_OBJECT
        void run() override;

public:
    Inferer();

    void infer();

    SDModel* model;
    Upscaler* esrgan;

    Axodox::MachineLearning::StableDiffusionOptions options;
    Axodox::Threading::async_operation_source* asyncSrc;

    QImage inImg;
    QImage inMask;
    std::string prompt;
    std::string negPrompt;
    uint32_t batchCount;
    bool randSeed;

    int32_t getStepsDone();


signals:
    void done(QImage Img, SDJobType jobType);
    void threadDone();
};

#endif