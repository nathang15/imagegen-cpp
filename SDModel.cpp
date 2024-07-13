#include "SDModel.h"
#include "Collections/Hasher.h"
#include "Infrastructure/BitwiseOperations.h"
#include "Infrastructure/DependencyContainer.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include "SDModel.h"

using namespace Axodox::Graphics;
using namespace Axodox::MachineLearning;
using namespace Axodox::Collections;
using json = nlohmann::json;
using namespace std;

void SDModel::getPredType(const std::string& modelPath)
{
	string schedulerConfigPath = "scheduler/scheduler_config.json";

	if (!(modelPath[modelPath.size() - 1] == '/' || modelPath[modelPath.size() - 1] == '\\'))
		schedulerConfigPath = "/" + schedulerConfigPath;

	ifstream i(modelPath + schedulerConfigPath);

	json schedulerConfig;
	i >> schedulerConfig;

	i.close();

	string predTypeString = schedulerConfig["prediction_type"].get<string>();

	if (predTypeString == "v_prediction")
		predType = StableDiffusionSchedulerPredictionType::V;
	else
		predType = StableDiffusionSchedulerPredictionType::Epsilon;
}

void SDModel::createTextEmbeddings(const std::string& prompt, const std::string& negPrompt, Axodox::MachineLearning::StableDiffusionOptions& options, Axodox::MachineLearning::ScheduledTensor* schTensor)
{
	auto encodedNegPrompt = txtEmbedder->SchedulePrompt(negPrompt, options.stepCount);
	auto encodedPrompt = txtEmbedder->SchedulePrompt(prompt, options.stepCount);

	options.TextEmbeddings.Weights.reserve(encodedNegPrompt[0].Weights.size() + encodedPrompt[0].Weights.size());
	for (auto weight : encodedNegPrompt[0].Weights) options.TextEmbeddings.Weights.push_back(-weight);
	for (auto weight : encodedPrompt[0].Weights) options.TextEmbeddings.Weights.push_back(weight);

	ScheduledTensor tensor = *schTensor;
	trivial_map<pair<void*, void*>, shared_ptr<EncodedText>> embeddingBuffer;
	for (auto i = 0u; i < options.StepCount; i++)
	{
		auto& concatTensor = embeddingBuffer[{encodedNegPrompt[i].Tensor.get(), encodedPrompt[i].Tensor.get() }];
		if (!concatTensor)
		{
			concatTensor = make_shared<EncodedText>(encodedNegPrompt[i].Tensor->Concat(*encodedPrompt[i].Tensor));
		}

		tensor[i] = concatTensor;
	}
	options.TextEmbeddings.Tensor = tensor;
}

void SDModel::loadVAEEncoder()
{
	VAE_E = std::make_unique<VaeEncoder>(*env);
}

SDModel::SDModel()
{
	Loaded = false;
}

void SDModel::destroy()
{
	try
	{
		env.reset();
		txtEmbedder.reset();
		uNet.reset();
		VAE_D.reset();
		VAE_E.reset();
	} catch (...)
	{
		//ERROR
	}
}

bool SDModel::load(const std::string& modelPath)
{
	if (Loaded)
		destroy();

	if (env)
		env.reset();

	env = make_unique<OnnxEnvironment>(modelPath);
	txtEmbedder = make_unique<TextEmbedder>(*env);
	uNet = make_unique<StableDiffusionInferer>(*env);

	getPredType(modelPath);
	Loaded = true;
	return true;
}

Axodox::MachineLearning::Tensor SDModel::encodeImageVAE(const Axodox::Graphics::TextureData& texData)
{
	if (!VAE_E)
		loadVAEEncoder();

	Tensor inpTexTensor = Tensor::FromTextureData(texData.ToFormat(DXGI_FORMAT_B8G8R8X8_UNORM_SRGB), ColorNormalization::LinearPlusMinusOne);

	return VAE_E->EncodeVae(inpTexTensor);
}

std::vector<Axodox::Collections::aligned_vector<uint8_t>> SDModel::doTxt2Img(const std::string& prompt, const std::string& negPrompt, Axodox::MachineLearning::StableDiffusionOptions options, Axodox::Threading::async_operation_source* opSrc)
{
	options.PredictionType = predType;
	ScheduledTensor scheduledEmbedTensor(options.StepCount);

	vector<aligned_vector<uint8_t>> imgBuffers;

	createTextEmbeddings(prompt, negPrompt, options, &scheduledEmbedTensor);

	auto x = uNet->RunInference(options, opSrc);

	if (opSrc->is_cancelled())
		return vector<aligned_vector<uint8_t>>{};

	x = VAE_D->DecodeVae(x);

	TextureData d;

	auto imgTextures = x.ToTextureData(ColorNormalization::LinearPlusMinusOne);

	for (auto& imgTexture : imgTextures)
	{
		auto imgBuffer = imgTexture.ToFormat(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB).Buffer;
		imgBuffers.push_back(imgBuffer);
	}

	return imgBuffers;
}

void SDModel::releaseDebugController()
{
	if (debugController) debugController->Release();
}

void SDModel::loadMinimal()
{
	env = make_unique<OnnxEnvironment>("");
}

SDModel::~SDModel()
{
	destroy();
}
