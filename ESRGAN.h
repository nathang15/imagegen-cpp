#pragma once
#include "Include/Axodox.MachineLearning.h"


namespace Axodox::MachineLearning
{
	class ESRGAN
	{
		static inline const Infrastructure::logger _logger{ "ESRGAN" };

	public:
		ESRGAN(OnnxEnvironment& environment, const std::string& ModelPath);

		Tensor Upscale(const Tensor& ImageChunk, Threading::async_operation_source* async = nullptr);

		void Evict();
	private:
		OnnxEnvironment& _environment;
		Ort::Session _session;
		bool _isUsingFloat16;
	};
}