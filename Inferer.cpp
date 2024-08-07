#include "inferer.h"
#include <iostream>
#include <random>
#include <QDebug>
#include "QtAxodoxInteropCommon.hpp"

using namespace Axodox::Graphics;
using namespace Axodox::MachineLearning;
using namespace Axodox::Collections;

using namespace QtAxInterop;
void Inferer::run()
{
    DoInference();
}

Inferer::Inferer() {
    AsyncSrc = nullptr;
    EsrGan = nullptr;
    Model = nullptr;
}

uint32_t getRandomUint32() {
    std::random_device rd;

    std::mt19937 gen(rd());

    std::uniform_int_distribution<uint32_t> _rand_distrub{};

    return _rand_distrub(gen);
}



void Inferer::DoInference()
{

    StableDiffusionJobType CurrentJobType = StableDiffusionJobType::Txt2Img;

    if (EsrGan)
    {

        CurrentJobType = StableDiffusionJobType::Upscale;

        QImage  UpsImg = EsrGan->UpscaleImg(InputImage, 256, 48, AsyncSrc);


        emit Done(UpsImg.copy(), CurrentJobType);

        emit ThreadFinished();
        return;
    }


    if (!InputImage.isNull())
    {
        CurrentJobType = StableDiffusionJobType::Img2Img;

        QImage ScaledImage = InputImage.scaled(Opts.Width, Opts.Height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

        TextureData TexDat;

        InterOpHelper::QImageToTextureData(ScaledImage, TexDat);


        try {
            Opts.LatentInput = Model->EncodeImageVAE(TexDat);

        }
        catch (std::exception& Ex) {

            qDebug() << "Failed to make latent input: " << Ex.what();
        }


        if (!InputMask.isNull())
        {
            QImage ScaledMask = InputMask.scaled(Opts.Width, Opts.Height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
                .scaled(Opts.Width / 8, Opts.Height / 8, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

            TextureData MaskTexDat;
            InterOpHelper::QImageToTextureData(ScaledMask, MaskTexDat);

            Opts.MaskInput = Tensor::FromTextureData(MaskTexDat.ToFormat(DXGI_FORMAT_R8_UNORM), ColorNormalization::LinearZeroToOne);

        }

    }




    for (uint32_t i = 0; i < BatchCount; i++)
    {

        if (RandomSeed) {
            Opts.Seed = getRandomUint32();
        }


        auto Buffs = Model->DoTxt2Img(Prompt, NegativePrompt, Opts, AsyncSrc);
        if (!Buffs.size()) {

            emit ThreadFinished();
            return;
        }

        int bytesPerPixel = 4; // 4 for RGBA, 3 for RGB
        QImage::Format format = QImage::Format_RGBA8888;

        for (auto& Buff : Buffs)
        {

            QImage image(Buff.data(), Opts.Width, Opts.Height, Opts.Width * bytesPerPixel, format);


            emit Done(image.copy(), CurrentJobType);
        }



    }

    emit ThreadFinished();

}

int32_t Inferer::GetStepsDone()
{

    if (AsyncSrc)
        return ((int32_t)std::round(AsyncSrc->state().progress)) * 100;

    return -1;
}