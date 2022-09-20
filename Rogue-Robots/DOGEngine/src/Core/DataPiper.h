#pragma once

namespace DOG::piper
{
	struct TempEntity
	{
		DirectX::SimpleMath::Matrix worldMatrix = DirectX::SimpleMath::Matrix::Identity;
		u64 modelID{ 0 };
	};

	struct PipedData
	{
		DirectX::XMMATRIX viewMat;
		DirectX::XMMATRIX* projMat;

		std::vector<TempEntity> entitiesToRender;
	};

	void SetPipe(PipedData* data);
	const PipedData* GetPipe();

}