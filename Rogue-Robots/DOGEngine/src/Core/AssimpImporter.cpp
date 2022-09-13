#include "AssimpImporter.h"
#include <Assimp/scene.h>           // Output data structure
#include <Assimp/Importer.hpp>      // C++ importer interface
#include <Assimp/postprocess.h>     // Post processing flags
#include <Assimp/pbrmaterial.h>

namespace DOG
{
	ImportedMaterial ExtractMaterial(const aiMaterial* aiMat, const std::string& directory)
	{
		ImportedMaterial importedMat;

		aiString materialName;
		if (!aiMat->Get(AI_MATKEY_NAME, materialName))
		{
			importedMat.materialName = materialName.C_Str();
			//std::cout << materialName.C_Str() << std::endl;
		}

		float metallicFactor;
		if (!aiMat->Get(AI_MATKEY_METALLIC_FACTOR, metallicFactor))
		{
			importedMat.metallicFactor = metallicFactor;
		}
		float roughnessFactor;
		if (!aiMat->Get(AI_MATKEY_METALLIC_FACTOR, roughnessFactor))
		{
			importedMat.roughnessFactor = roughnessFactor;
		}

		aiColor4D albedoFactor(0.0f, 0.0f, 0.0f, 0.0f);
		if (!aiMat->Get(AI_MATKEY_BASE_COLOR, albedoFactor))
		{
			importedMat.albedoFactor[0] = albedoFactor[0];
			importedMat.albedoFactor[1] = albedoFactor[1];
			importedMat.albedoFactor[2] = albedoFactor[2];
			importedMat.albedoFactor[3] = albedoFactor[3];
		}

		aiColor3D emissiveFactor(0.0f, 0.0f, 0.0f);
		if (!aiMat->Get(AI_MATKEY_COLOR_EMISSIVE, emissiveFactor))
		{
			importedMat.emissiveFactor[0] = emissiveFactor[0];
			importedMat.emissiveFactor[1] = emissiveFactor[1];
			importedMat.emissiveFactor[2] = emissiveFactor[2];
		}

		aiString textureFileName;
		if (!aiMat->GetTexture(AI_MATKEY_BASE_COLOR_TEXTURE, &textureFileName))
		{
			importedMat.albedoPath = directory + textureFileName.C_Str();
			std::cout << directory + textureFileName.C_Str() << std::endl;
		}

		if (!aiMat->GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE, &textureFileName))
		{
			importedMat.metallicRoughnessPath = directory + textureFileName.C_Str();
			std::cout << directory + textureFileName.C_Str() << std::endl;
		}

		if (!aiMat->GetTexture(aiTextureType::aiTextureType_EMISSIVE, 0, &textureFileName))
		{
			importedMat.emissivePath = directory + textureFileName.C_Str();
			std::cout << directory + textureFileName.C_Str() << std::endl;
		}

		if (!aiMat->GetTexture(aiTextureType::aiTextureType_NORMALS, 0, &textureFileName))
		{
			importedMat.normalMapPath = directory + textureFileName.C_Str();
			std::cout << directory + textureFileName.C_Str() << std::endl;
		}
		return importedMat;
	}

	AssimpImporter::AssimpImporter(const std::filesystem::path& path)
	{
		// Load assimp scene
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(
			path.relative_path().string().c_str(),
			aiProcess_Triangulate |

			// For Direct3D
			aiProcess_ConvertToLeftHanded |
			//aiProcess_FlipUVs |					// (0, 0) is top left
			//aiProcess_FlipWindingOrder |			// D3D front face is CW

			aiProcess_GenSmoothNormals |
			aiProcess_CalcTangentSpace |

			//aiProcess_PreTransformVertices

			// Extra flags (http://assimp.sourceforge.net/lib_html/postprocess_8h.html#a64795260b95f5a4b3f3dc1be4f52e410a444a6c9d8b63e6dc9e1e2e1edd3cbcd4)
			aiProcess_JoinIdenticalVertices |
			aiProcess_ImproveCacheLocality
		);

		if (!scene)
			assert(false);

		m_loadedModel = std::make_shared<ImportedModel>();
		m_loadedModel->mesh.vertexData[VertexAttribute::Position] = {};
		m_loadedModel->mesh.vertexData[VertexAttribute::UV] = {};
		m_loadedModel->mesh.vertexData[VertexAttribute::Normal] = {};
		m_loadedModel->mesh.vertexData[VertexAttribute::Tangent] = {};

		// Track material per submesh
		std::vector<u32> submesh_to_material_idx;
		std::vector<SubmeshMetadata>& submeshes = m_loadedModel->submeshes;

		// Load mesh
		{
			std::vector<u32>& indices = m_loadedModel->mesh.indices;
			std::vector<aiVector3D> positions, normals, tangents;
			std::vector<aiVector2D> uvs;

			// Reserve space
			{
				u32 total_verts{ 0 };
				for (u32 i = 0; i < scene->mNumMeshes; ++i)
					total_verts += scene->mMeshes[i]->mNumVertices;

				positions.reserve(total_verts);
				uvs.reserve(total_verts);
				normals.reserve(total_verts);
				tangents.reserve(total_verts);
			}

			// Go through all meshes
			for (uint32_t mesh_idx = 0; mesh_idx < scene->mNumMeshes; ++mesh_idx)
			{
				aiMesh* mesh = scene->mMeshes[mesh_idx];

				// Track submesh
				SubmeshMetadata submesh_md{};
				submesh_md.vertexStart = (u32)positions.size();
				submesh_md.vertexCount = mesh->mNumVertices;
				submesh_md.indexStart = (u32)indices.size();
				submesh_md.indexCount = 0;

				// Count indices
				for (u32 face_idx = 0; face_idx < mesh->mNumFaces; ++face_idx)
				{
					const aiFace& face = mesh->mFaces[face_idx];
					for (u32 index_idx = 0; index_idx < face.mNumIndices; ++index_idx)
						indices.push_back(face.mIndices[index_idx]);
					submesh_md.indexCount += face.mNumIndices;
				}

				// Grab per vertex data
				for (u32 vert_idx = 0; vert_idx < mesh->mNumVertices; ++vert_idx)
				{
					positions.push_back({ mesh->mVertices[vert_idx].x, mesh->mVertices[vert_idx].y, mesh->mVertices[vert_idx].z });

					if (mesh->HasTextureCoords(0))
						uvs.push_back({ mesh->mTextureCoords[0][vert_idx].x, mesh->mTextureCoords[0][vert_idx].y });

					if (mesh->HasNormals())
						normals.push_back({ mesh->mNormals[vert_idx].x, mesh->mNormals[vert_idx].y, mesh->mNormals[vert_idx].z });

					if (mesh->HasTangentsAndBitangents())
					{
						tangents.push_back({ mesh->mTangents[vert_idx].x, mesh->mTangents[vert_idx].y, mesh->mTangents[vert_idx].z });
						//bitangents.push_back({ mesh->mBitangents[vert_idx].x, mesh->mBitangents[vert_idx].y, mesh->mBitangents[vert_idx].z });
					}
				}

				// Track material
				submesh_to_material_idx.push_back(mesh->mMaterialIndex);

				// Track submesh
				submeshes.push_back(submesh_md);
			}

			// Resize standardized buffers
			m_loadedModel->mesh.vertexData[VertexAttribute::Position].resize(positions.size() * sizeof(positions[0]));
			m_loadedModel->mesh.vertexData[VertexAttribute::UV].resize(uvs.size() * sizeof(uvs[0]));
			m_loadedModel->mesh.vertexData[VertexAttribute::Normal].resize(normals.size() * sizeof(normals[0]));
			m_loadedModel->mesh.vertexData[VertexAttribute::Tangent].resize(tangents.size() * sizeof(tangents[0]));

			// Copy to standardized buffers
			std::memcpy(m_loadedModel->mesh.vertexData[VertexAttribute::Position].data(), positions.data(), positions.size() * sizeof(positions[0]));
			std::memcpy(m_loadedModel->mesh.vertexData[VertexAttribute::UV].data(), uvs.data(), uvs.size() * sizeof(uvs[0]));
			std::memcpy(m_loadedModel->mesh.vertexData[VertexAttribute::Normal].data(), normals.data(), normals.size() * sizeof(normals[0]));
			std::memcpy(m_loadedModel->mesh.vertexData[VertexAttribute::Tangent].data(), tangents.data(), tangents.size() * sizeof(tangents[0]));
		}

		// Sanity check
		assert(submeshes.size() == submesh_to_material_idx.size());

		// Extract material load data
		const auto directory = path.parent_path().string();
		for (auto mat_id : submesh_to_material_idx)
		{
			const aiMaterial* material = scene->mMaterials[mat_id];

			// Save material
			m_loadedModel->materials.push_back(ExtractMaterial(material, directory + "/"));
		}
	}
}