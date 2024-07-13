#pragma once
#include "Include/Axodox.MachineLearning.h"

namespace Axodox::MachineLearning {
	class ESRGAN {
		static inline const Infrastructure::logger _logger{ "ESRGAN" };
	public:
		ESRGAN(OnnxEnvironment& env, const std::string& modelPath);

		Tensor upscale(const Tensor& imgChunk, Threading::async_operation_source* async = nullptr);

		void Evict();
	};
private:
	OnxxEnvironment& _env;
	Ort::Session _session;
	bool _isUsingFloat16;
}
}