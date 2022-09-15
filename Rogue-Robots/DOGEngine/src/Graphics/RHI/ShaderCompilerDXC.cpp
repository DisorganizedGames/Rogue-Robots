#include "ShaderCompilerDXC.h"
#include "DirectXShaderCompiler/dxcapi.h"

namespace DOG::gfx
{

	template <typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	ShaderCompilerDXC::ShaderCompilerDXC(ShaderModel model) :
		m_shaderModel(model)
	{
		// Grab interfaces
		HRESULT hr = S_OK;

		hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&m_library));
		if (FAILED(hr))
			throw std::runtime_error("Failed to intiialize IDxcLibrary");

		hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_compiler));
		if (FAILED(hr))
			throw std::runtime_error("Failed to initialize IDxcCompiler");

		hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_utils));
		if (FAILED(hr))
			throw std::runtime_error("Failed to initialize IDxcUtils");

		// Grab default include handler
		m_utils->CreateDefaultIncludeHandler(m_defIncHandler.GetAddressOf());
	}

	ShaderCompilerDXC::~ShaderCompilerDXC()
	{
	}

	std::shared_ptr<CompiledShader> ShaderCompilerDXC::CompileFromFile(std::filesystem::path relPath, ShaderType type, const std::string& entryPoint)
	{
		std::wstring entry_wstr = std::filesystem::path(entryPoint).wstring();

		std::cout << std::filesystem::current_path().string()  << "\n";

#ifdef _DEBUG
		std::string dir = "Assets/Shaders/";
#else
		// Waiting for Release with Debug!
		// Currently, release looks for shaders inside DOGEngine
		std::string dir = "Assets/Shaders/";
		//std::string dir = "Shaders\\";
#endif

		// Prepend directory
		relPath = std::filesystem::path(dir + relPath.string());

		std::wstring profile = GrabProfile(type);

		// Create blob to store compiled data
		uint32_t codePage = CP_UTF8;
		ComPtr<IDxcBlobEncoding> sourceBlob;

		HRESULT hr;
		hr = m_library->CreateBlobFromFile(relPath.c_str(), &codePage, &sourceBlob);
		assert(SUCCEEDED(hr));

		// Compile
		ComPtr<IDxcOperationResult> result;
		hr = m_compiler->Compile(
			sourceBlob.Get(), // pSource
			relPath.c_str(),// pSourceName
			entry_wstr.c_str(), // pEntryPoint
			profile.c_str(), // pTargetProfile
			NULL, 0, // pArguments, argCount
			NULL, 0, // pDefines, defineCount
			m_defIncHandler.Get(),
			result.GetAddressOf()); // ppResult

		if (SUCCEEDED(hr))
			result->GetStatus(&hr);

		// Check error
		if (FAILED(hr))
		{
			ComPtr<IDxcBlobEncoding> errors;
			hr = result->GetErrorBuffer(&errors);
			if (SUCCEEDED(hr) && errors)
			{
				wprintf(L"Compilation failed with errors:\n%hs\n",
					(const char*)errors->GetBufferPointer());
				assert(false);
			}
		}

		ComPtr<IDxcBlob> res;
		hr = result->GetResult(res.GetAddressOf());
		if (FAILED(hr))
			assert(false);

		return std::make_shared<CompiledShader>(res->GetBufferPointer(), res->GetBufferSize(), type);
	}

	std::wstring ShaderCompilerDXC::GrabProfile(ShaderType shader_type)
	{
		std::wstring profile;
		switch (shader_type)
		{
		case ShaderType::Vertex:
			profile = L"vs";
			break;
		case ShaderType::Pixel:
			profile = L"ps";
			break;
		case ShaderType::Geometry:
			profile = L"gs";
			break;
		case ShaderType::Hull:
			profile = L"hs";
			break;
		case ShaderType::Domain:
			profile = L"ds";
			break;
		case ShaderType::Compute:
			profile = L"cs";
			break;
		default:
			assert(false);
		}
		profile += L"_";

		switch (m_shaderModel)
		{
		case ShaderModel::SM_6_6:
			profile += L"6_6";
			break;
		default:
			assert(false);
		}
		return profile;
	}
}