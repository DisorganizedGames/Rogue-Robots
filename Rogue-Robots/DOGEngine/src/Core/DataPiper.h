#pragma once

namespace DOG::piper
{
	struct PipedData
	{
		DirectX::XMMATRIX viewMat;
		DirectX::XMMATRIX* projMat;
	};

	void SetPipe(PipedData* data);
	const PipedData* GetPipe();

}