#pragma once
#include <wrl/client.h>
#include <filesystem>
#include "Types/ShaderTypes.h"

struct IDxcLibrary;
struct IDxcUtils;
struct IDxcCompiler;
struct IDxcIncludeHandler;

namespace DOG::gfx
{
	class ShaderCompilerDXC
	{
	public:
		ShaderCompilerDXC(ShaderModel model = ShaderModel::SM_6_6);
		~ShaderCompilerDXC();

		std::shared_ptr<CompiledShader> CompileFromFile(
			std::filesystem::path relPath,
			ShaderType type,
			const std::string& entryPoint = "main");

	private:
		std::wstring GrabProfile(ShaderType shader_type);

	private:
		ShaderModel m_shaderModel;

		Microsoft::WRL::ComPtr<IDxcLibrary> m_library;
		Microsoft::WRL::ComPtr<IDxcUtils> m_utils;
		Microsoft::WRL::ComPtr<IDxcCompiler> m_compiler;
		Microsoft::WRL::ComPtr<IDxcIncludeHandler> m_defIncHandler;

	};
}