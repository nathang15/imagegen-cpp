#include "Inferer.h"
#include <iostream>
#include <random>
#include <QDebug>
#include "InteropAxodoxCommon.hpp"

using namespace Axodox::Graphics;
using namespace Axodox::MachineLearning;
using namespace Axodox::Collections;
using namespace QtAxInterop;

void Inferer::run()
{
    infer();
}

Inferer::Inferer()
{
    asyncSrc = nullptr;
    esrgan = nullptr;
    model = nullptr;
}

uint32_t getRandomUint32()
{
    std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<uint32_t> dis(0, UINT32_MAX);
	return dis(gen);
}

void Inferer::infer()
{
    SDJobType curJobType = SDJobType::txt2Img;

    if (Esrgan)
    {
        curJobType = SDJobType::upscale;
        QImage upsImg = Esrgan->upscaleImg(inImg, 256, 48, asyncSrc);

        emit done(upsImg.copy(), curJobType);
        emit threadDone();
        return;
    }

    if (!inImg.isNull())
    {
        curJobType = SDJobType::img2Img;
        QImage scaledImg = inImg.scaled(options.Width, options.Height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

        TextureData texDat;
        InterOpHelper::QImageToTextureData(scaledImg, texDat);

        try {
            options.LatentInput = model->encodeImageVAE(texDat)
        }
        catch (std::exception& e)
        {
            qDebug() << "Failed to make latent input: " << e.what();
        }

        if (!inMask.isNull())
        {
            QImage scaledMask = inMask.scaled(options.Width, options.Height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
                .scaled(options.Width / 8, options.Height / 8, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

            TextureData maskTexDat;
            InterOpHelper::QImageToTextureData(scaledMask, maskTexDat);

            Opts.MaskInput = Tensor::FromTextureData(maskTexDat.ToFormat(DXGI_FORMAT_R8_UNORM), ColorNormalization::LinearZeroToOne);
        }
    }

    for (uint32_t i = 0; i < batchCount; i++)
    {
        if (randSeed)
        {
            options.Seed = getRandomUint32();
        }

        auto buffs = model->doTxt2Img(prompt, negPrompt, options, asyncSrc);
        if (!buffs.size())
        {
            emit threadDone();
            return;
        }

        int bytesPerPix = 4;
        QImage::Format format = QImage::Format_RGBA8888;
        for (auto& buff : buffs)
        {
            QImage img(buff.data(), options.Width, options.Height, options.Width * bytesPerPix, format);
            emit Done(img.copy(), curJobType);
        }
    }
    emit threadDone();
}

int32_t Inferer::GetStepsDone()
{
    if (asyncSrc)
        return ((int32_t)std::round(asyncSrc->state().progress)) * 100;

    return -1;
}