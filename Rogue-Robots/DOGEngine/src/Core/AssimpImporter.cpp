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
		}

		if (!aiMat->GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE, &textureFileName))
		{
			importedMat.metallicRoughnessPath = directory + textureFileName.C_Str();
		}

		if (!aiMat->GetTexture(aiTextureType::aiTextureType_EMISSIVE, 0, &textureFileName))
		{
			importedMat.emissivePath = directory + textureFileName.C_Str();
		}

		if (!aiMat->GetTexture(aiTextureType::aiTextureType_NORMALS, 0, &textureFileName))
		{
			importedMat.normalMapPath = directory + textureFileName.C_Str();
		}
		return importedMat;
	}

	void PopulateNodeVector(aiNode* node, std::vector<aiNode*>& nodes)
	{
		nodes.push_back(node);

		for (u32 i = 0; i < node->mNumChildren; i++)
			PopulateNodeVector(node->mChildren[i], nodes);
	}

	void AddNode(std::unordered_map<std::string, i32>& nameToJointIdx, const std::unordered_map<std::string, aiBone*>& nameToJoint,
		std::vector<JointNode>& nodeArray, const aiNode* node, i32 parentIdx)
	{
		JointNode n = {};
		std::string nodeName = node->mName.C_Str();

		const bool hasJoint = (nameToJoint.find(nodeName) != nameToJoint.end());
		if (hasJoint)
		{
			n.jointIdx = (i32)nameToJointIdx.size();
			nameToJointIdx.insert({ nodeName, n.jointIdx });
		}

		// Store Node if it is associated with bone or is root of bone hierarchy
		if (hasJoint || parentIdx == 0)
		{
			DirectX::XMStoreFloat4x4(&n.transformation, DirectX::XMMATRIX(&node->mTransformation.a1));
			n.parentIdx = parentIdx;
			n.name = node->mName.C_Str();
			parentIdx = (i32)nodeArray.size();
			nodeArray.push_back(n);
		}
		// Recur for node children
		for (u32 i = 0; i < node->mNumChildren; i++)
			AddNode(nameToJointIdx, nameToJoint, nodeArray, node->mChildren[i], parentIdx);
	}

	ImportedAnimation ImportAnimation(const aiScene* scene, std::shared_ptr<ImportedModel>& m_model)
	{
		std::unordered_map<std::string, aiBone*> nameToJoint;
		std::unordered_map<std::string, i32> nameToJointIdx;
		std::vector<aiNode*> allNodes;

		ImportedAnimation importedAnim = {};
		std::vector<JointNode>& nodeArray = importedAnim.nodes;
		std::vector<dxf4x4>& boneArray = importedAnim.jointOffsets;

		// Add all bones to bone map
		for (u32 i = 0; i < scene->mNumMeshes; i++)
			for (u32 j = 0; j < scene->mMeshes[i]->mNumBones; j++)
				nameToJoint.insert({ scene->mMeshes[i]->mBones[j]->mName.C_Str(), scene->mMeshes[i]->mBones[j] });

		// Recur through node tree and store all nodes in vector
		PopulateNodeVector(scene->mRootNode, allNodes);

		// Store model Root Node
		nodeArray.push_back(JointNode{});
		nodeArray.back().name = allNodes[0]->mName.C_Str();
		DirectX::XMStoreFloat4x4(&nodeArray.back().transformation, DirectX::XMMATRIX(&allNodes[0]->mTransformation.a1));

		// Find and store Root of bone Hierarchy (Parent of first node associated with a bone)
		u32 boneRootIdx = {};
		for (boneRootIdx = 0; boneRootIdx < allNodes.size(); boneRootIdx++)
			if (nameToJoint.find(allNodes[boneRootIdx]->mName.C_Str()) != nameToJoint.end())
				break;
		const aiNode* boneRootNode = allNodes[boneRootIdx]->mParent;
		// Add bone hierarchy
		AddNode(nameToJointIdx, nameToJoint, nodeArray, boneRootNode, 0);

		// Store bone offset matrices
		boneArray.assign(nameToJoint.size(), dxf4x4{});
		for (auto& n : allNodes)
			if (nameToJoint.find(n->mName.C_Str()) != nameToJoint.end())
				DirectX::XMStoreFloat4x4(&boneArray[nameToJointIdx.at(n->mName.C_Str())],
					DirectX::XMMATRIX(&nameToJoint.at(n->mName.C_Str())->mOffsetMatrix.a1));

		// Store vertices BoneWeight and Indices
		m_model->mesh.vertexData[VertexAttribute::BlendData] = {};

		struct BoneIndices{
			i32 indices[4] = { -1, -1, -1, -1 };
		};
		struct BoneWeights{
			f32 weights[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		};
		std::vector<BoneIndices> boneIndices;
		std::vector<BoneWeights> boneWeights;

		// Assign default four bone influences per vertice.
		{
			u32 total_vertices{ 0 };
			for (u32 i = 0; i < scene->mNumMeshes; ++i)
				total_vertices += scene->mMeshes[i]->mNumVertices;

			boneIndices.assign(total_vertices, {});
			boneWeights.assign(total_vertices, {});
		}

		std::vector<u32> boneWeightIdx(boneIndices.size(), 0);
		u32 submesh_start = 0;
		// Go through meshes and set vertice bone indices/weights
		for (u32 mesh_idx = 0; mesh_idx < scene->mNumMeshes; mesh_idx++)
		{
			aiMesh* mesh = scene->mMeshes[mesh_idx];

			for (u32 bone_idx = 0; bone_idx < mesh->mNumBones; bone_idx++)
			{
				aiBone* bone = mesh->mBones[bone_idx];

				for (u32 weight_idx = 0; weight_idx < bone->mNumWeights; weight_idx++)
				{
					auto vWeight = bone->mWeights[weight_idx];
					u32 vertex = submesh_start + vWeight.mVertexId;

					boneIndices[vertex].indices[boneWeightIdx[vertex]] = nameToJointIdx[bone->mName.C_Str()];
					boneWeights[vertex].weights[boneWeightIdx[vertex]] = vWeight.mWeight;

					boneWeightIdx[vertex]++;
				}
			}
			submesh_start += mesh->mNumVertices;
		}
		// normalize weights
		for (auto& w : boneWeights)
		{
			auto sum = w.weights[0] + w.weights[1] + w.weights[2] + w.weights[3];
			for (u32 i = 0; i < 4; i++)
				w.weights[i] /= sum;
		}

		std::vector<BlendData> blendData;
		blendData.reserve(boneWeights.size() * 4);
		for (u32 i = 0; i < boneWeights.size(); i++)
		{
			blendData.push_back({ boneIndices[i].indices[0], boneWeights[i].weights[0] });
			blendData.push_back({ boneIndices[i].indices[1], boneWeights[i].weights[1] });
			blendData.push_back({ boneIndices[i].indices[2], boneWeights[i].weights[2] });
			blendData.push_back({ boneIndices[i].indices[3], boneWeights[i].weights[3] });
		}
		
		// resize buffer
		m_model->mesh.vertexData[VertexAttribute::BlendData].resize(blendData.size() * sizeof(blendData[0]));
		std::memcpy(m_model->mesh.vertexData[VertexAttribute::BlendData].data(), blendData.data(), blendData.size() * sizeof(blendData[0]));

		// Import animation data
		std::unordered_map<std::string, u32> nameToNodeIdx;
		for (u32 i = 0; i < nodeArray.size(); i++)
			nameToNodeIdx.insert({ nodeArray[i].name, i });
		importedAnim.animations.reserve(scene->mNumAnimations);
		for (u32 i = 0; i < scene->mNumAnimations; i++)
		{
			const auto animation = scene->mAnimations[i];
			importedAnim.animations.push_back({});
			importedAnim.animations.back().duration = (f32)animation->mDuration / (f32)animation->mTicksPerSecond;
			importedAnim.animations.back().ticksPerSec = (f32)animation->mTicksPerSecond;
			importedAnim.animations.back().ticks = (f32)animation->mDuration;
			importedAnim.animations.back().name = animation->mName.C_Str();

			for (u32 ch_i = 0; ch_i < scene->mAnimations[i]->mNumChannels; ch_i++)
			{
				const auto channel = scene->mAnimations[i]->mChannels[ch_i];

				std::vector<AnimationKey> posKeys;
				std::vector<AnimationKey> rotKeys;
				std::vector<AnimationKey> scaKeys;
				posKeys.reserve(channel->mNumPositionKeys);
				rotKeys.reserve(channel->mNumRotationKeys);
				scaKeys.reserve(channel->mNumScalingKeys);

				for (u32 k = 0; k < posKeys.capacity(); k++)
				{
					const auto aiKey = channel->mPositionKeys[k];
					posKeys.push_back({channel->mNodeName.C_Str(), (f32)aiKey.mTime, {aiKey.mValue.x, aiKey.mValue.y, aiKey.mValue.z, 0.0f} });
				}
				for (size_t k = 0; k < scaKeys.capacity(); k++)
				{
					const auto aiKey = channel->mScalingKeys[k];
					scaKeys.push_back({ channel->mNodeName.C_Str(), (f32)aiKey.mTime, {aiKey.mValue.x, aiKey.mValue.y, aiKey.mValue.z, 0.0f} });
				}
				for (size_t k = 0; k < rotKeys.capacity(); k++)
				{
					const auto aiKey = channel->mRotationKeys[k];
					rotKeys.push_back({ channel->mNodeName.C_Str(), (f32)aiKey.mTime, {aiKey.mValue.x, aiKey.mValue.y, aiKey.mValue.z, aiKey.mValue.w} });
				}

				std::string nodeName = channel->mNodeName.C_Str();
				if (nameToNodeIdx.find(nodeName) == nameToNodeIdx.end())
					nameToNodeIdx.insert({ nodeName, -1 });

				i32 nodeID = nameToNodeIdx.at(nodeName);
				importedAnim.animations.back().scaKeys.insert({nodeID, scaKeys});
				importedAnim.animations.back().rotKeys.insert({nodeID, rotKeys});
				importedAnim.animations.back().posKeys.insert({nodeID, posKeys});
			}
		}

		return importedAnim;
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
				if(scene->HasAnimations())
				{
					submesh_md.blendStart = (u32)positions.size();
					submesh_md.blendCount = mesh->mNumVertices;
				}
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
		const auto directory = path.parent_path().string() + "/";
		for (auto mat_id : submesh_to_material_idx)
		{
			const aiMaterial* material = scene->mMaterials[mat_id];

			// Save material
			m_loadedModel->materials.push_back(ExtractMaterial(material, directory + "/"));
		}

		if (scene->HasAnimations())
			m_loadedModel->animation = ImportAnimation(scene, m_loadedModel);
	}
}