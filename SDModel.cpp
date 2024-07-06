#include "stablediffusionmodel.h"
#include "Collections/Hasher.h"
#include "Infrastructure/BitwiseOperations.h"
#include "Infrastructure/DependencyContainer.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include "SDModel.h"

void SDModel::getPredType(const std::string& modelPath)
{
}

void SDModel::createTextEmbeddings(const std::string& prompt, const std::string& negPrompt, Axodox::MachineLearning::StableDiffusionOptions& options, Axodox::MachineLearning::ScheduledTensor* schTensor)
{
}

void SDModel::loadVAEEncoder()
{
}

SDModel::SDModel()
{
}

void SDModel::destroy()
{
}

bool SDModel::load(const std::string& modelPath)
{
	return false;
}

Axodox::MachineLearning::Tensor SDModel::encodeImageVAE(const Axodox::Graphics::TextureData& texData)
{
	return Axodox::MachineLearning::Tensor();
}

std::vector<Axodox::Collections::aligned_vector<uint8_t>> SDModel::doTxt2Img(const std::string& prompt, const std::string& negPrompt, Axodox::MachineLearning::StableDiffusionOptions options, Axodox::Threading::async_operation_source* opSrc)
{
	return std::vector<Axodox::Collections::aligned_vector<uint8_t>>();
}

void SDModel::releaseDebugController()
{
}

void SDModel::loadMinimal()
{
}

SDModel::~SDModel()
{
}
