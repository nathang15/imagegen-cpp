#ifndef INFERER_H
#define INFERER_H

#include <QThread>
#include "stablediffusionmodel.h"
#include "upscaler.h"
#include <QImage>

enum class StableDiffusionJobType {
    Txt2Img,
    Img2Img,
    Upscale
};
class Inferer : public QThread
{
    Q_OBJECT

        void run() override;



public:
    Inferer();

    void DoInference();

    StableDiffusionModel* Model;
    Upscaler* EsrGan;

    Axodox::MachineLearning::StableDiffusionOptions Opts;
    Axodox::Threading::async_operation_source* AsyncSrc;

    QImage InputImage;
    QImage InputMask;
    std::string Prompt;
    std::string NegativePrompt;
    uint32_t BatchCount;
    bool RandomSeed;

    int32_t GetStepsDone();


signals:
    void Done(QImage Img, StableDiffusionJobType JobType);
    void ThreadFinished();
};

#endif // INFERER_H