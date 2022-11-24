#pragma once
#include "../ECS/Component.h"
#include "Types/AssetTypes.h"

namespace DOG
{
	[[nodiscard]] SubmeshRenderer CreateSimpleModel(MaterialDesc material, ImportedMesh mesh, std::optional<DirectX::SimpleMath::Matrix> correctionMatrix) noexcept;
}
