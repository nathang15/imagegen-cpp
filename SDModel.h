#ifndef SDMODEL_H
#define SDMODEL_H
#include "Include/Axodox.MachineLearning.h"

class SDModel
{
private:
    std::unique_ptr<Axodox::MachineLearning::TextEmbedder> txtEmbedder;
    std::unique_ptr<Axodox::MachineLearning::StableDiffusionInferer> uNet;
    std::unique_ptr<Axodox::MachineLearning::VaeDecoder> VAE_D;
    std::unique_ptr<Axodox::MachineLearning::VaeEncoder> VAE_E;
    std::unique_ptr<Axodox::MachineLearning::OnnxEnvironment> env;
    ID3D12Debug* debugController;

    Axodox::MachineLearning::StableDiffusionSchedulerPredictionType predType;

    bool Loaded;

    void getPredType(const std::string& modelPath);
    void createTextEmbeddings(
        const std::string& prompt, 
        const std::string& negPrompt, 
        Axodox::MachineLearning::StableDiffusionOptions& options, 
        Axodox::MachineLearning::ScheduledTensor* schTensor);
    void loadVAEEncoder();

public:
    SDModel();

    void destroy();
    bool load(const std::string& modelPath);

    Axodox::MachineLearning::Tensor encodeImageVAE(const Axodox::Graphics::TextureData& texData);

    std::vector<Axodox::Collections::aligned_vector<uint8_t>> doTxt2Img(
        const std::string& prompt, 
        const std::string& negPrompt, 
        Axodox::MachineLearning::StableDiffusionOptions options, 
        Axodox::Threading::async_operation_source* opSrc = nullptr
    );

    void releaseDebugController();

    inline bool loaded() const { return loaded; }
    inline Axodox::MachineLearning::OnnxEnvironment* getEnv() { return env.get(); }
    void loadMinimal();

    ~SDModel();
};

#endif