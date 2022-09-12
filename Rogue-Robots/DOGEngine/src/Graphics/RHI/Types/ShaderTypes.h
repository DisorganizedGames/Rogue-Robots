#pragma once

namespace DOG::gfx
{

	enum class ShaderType
	{
		Vertex,
		Geometry,
		Hull,
		Domain,
		Pixel,
		Compute
	};

	enum class ShaderModel
	{
		SM_6_6
	};

	struct CompiledShader
	{
		std::vector<u8> blob;
		ShaderType shaderType{ ShaderType::Vertex };
		ShaderModel shaderModel{ ShaderModel::SM_6_6 };

		CompiledShader() = delete;

		CompiledShader(void* data, size_t size, ShaderType type, ShaderModel model = ShaderModel::SM_6_6) :
			shaderType(type), shaderModel(model)
		{
			blob.resize(size);
			uint8_t* start = (uint8_t*)data;
			uint8_t* end = start + size;
			std::copy(start, end, blob.data());
		}
	};
}