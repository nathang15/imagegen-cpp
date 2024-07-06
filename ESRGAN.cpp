#include "ESRGAN.h"
using namespace Axodox::Infrastructure;
using namespace Ort;
using namespace std;

namespace Axodox::MachineLearning
{
    ESRGAN::ESRGAN(OnnxEnvironment& env, const std::string& modelPath) :
        _env(env),
        _session(env->CreateSession(modelPath))
    {

        auto metadata = OnnxModelMetadata::Create(_env, _session);

        _session.Evict();
        _logger.log(log_severity::information, "Loaded");
        _isUsingFloat16 = false;
    }

    Tensor ESRGAN::upscale(const Tensor& imgChunk, Threading::async_operation_source* async)
    {
        IoBinding bindings{ _session };
        bindings.BindInput("input_img", imgChunk.ToOrtValue());
        bindings.BindOutput("upscaled", _env->MemoryInfo());

        _session.Run({}, bindings);

        auto outputValues = bindings.GetOutputValues();
        auto result = Tensor::FromOrtValue(outputValues[0]).ToSingle();

        return result;
    }

    void ESRGAN::Evict()
    {
        _session.Evict();
    }
}