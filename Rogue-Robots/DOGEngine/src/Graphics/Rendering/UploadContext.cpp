#include "UploadContext.h"
#include "../RHI/RenderDevice.h"

namespace DOG::gfx
{
	UploadContext::UploadContext(RenderDevice* rd, u32 stagingSizePerVersion, u8 maxVersions, QueueType targetQueue) :
		m_rd(rd),
		m_maxVersions(maxVersions),
		m_targetQueue(targetQueue)
	{
		assert(targetQueue != QueueType::Compute);

		m_storage.resize(maxVersions);

		// Setup versioned staging buffers
		for (u32 i = 0; i < maxVersions; ++i)
		{
			auto& version = m_storage[i];
			version.staging = m_rd->CreateBuffer(BufferDesc(MemoryType::Upload, stagingSizePerVersion));
			version.vator = BumpAllocator(stagingSizePerVersion, m_rd->Map(version.staging, 0));
			version.copyCmdl = m_rd->AllocateCommandList(targetQueue);
		}
	}

	void UploadContext::PushUpload(Buffer dst, u32 dstOffset, void* data, u32 dataSize)
	{
		auto& version = m_storage[m_currVersion];

		// Reset version.. safely..
		if (version.copyDoneReceipt)
		{
			m_rd->WaitForGPU(*version.copyDoneReceipt);
		
			version.copyDoneReceipt = std::nullopt;
			version.vator.Clear();

			// Recycle command list
			m_rd->RecycleCommandList(version.copyCmdl);
			version.copyCmdl = m_rd->AllocateCommandList(m_targetQueue);
		}

		// Allocate staging memory
		auto [memory, offsetFromBase] = version.vator.AllocateWithOffset(dataSize);

		// Copy to staging
		std::memcpy(memory, data, dataSize);

		// Record GPU-GPU copy
		m_rd->Cmd_CopyBuffer(version.copyCmdl,
			dst, dstOffset, version.staging, (u32)offsetFromBase, dataSize);
	}

	void UploadContext::PushUploadToTexture(
		Texture dst, 
		u32 dstSubresource, 
		std::tuple<u32, u32, u32> dstTopLeft,
		void* srcData, 
		DXGI_FORMAT srcFormat, 
		u32 srcWidth,
		u32 srcHeight, 
		u32 srcDepth, 
		u32 srcRowPitch)
	{
		auto& version = m_storage[m_currVersion];

		// Reset version.. safely..
		if (version.copyDoneReceipt)
		{
			m_rd->WaitForGPU(*version.copyDoneReceipt);

			version.copyDoneReceipt = std::nullopt;
			version.vator.Clear();

			// Recycle command list
			m_rd->RecycleCommandList(version.copyCmdl);
			version.copyCmdl = m_rd->AllocateCommandList(m_targetQueue);
		}

		const u32 totalSize = srcRowPitch * srcHeight;

		static constexpr u32 TEXALIGNMENT = 512;

		// Allocate staging memory
		auto [memory, offsetFromBase] = version.vator.AllocateWithOffset(totalSize, TEXALIGNMENT);

		// Copy to staging
		std::memcpy(memory, srcData, totalSize);

		// Record GPU-GPU copy
		m_rd->Cmd_CopyBufferToImage(version.copyCmdl,
			dst, 
			dstSubresource, 
			dstTopLeft, 
			version.staging, 
			(u32)offsetFromBase, 
			srcFormat, srcWidth, srcHeight, srcDepth, srcRowPitch);
	}

	std::optional<SyncReceipt> UploadContext::SubmitCopies()
	{
		auto& version = m_storage[m_currVersion];

		// Assert that PushUpload has been called before SubmitCopies and waited for the receipt
		assert(!version.copyDoneReceipt);

		//const bool generateSync = m_targetQueue == QueueType::Copy ? true : false;
		const bool generateSync = true;

		// Used to internally sync with next time m_currVersion occurs
		version.copyDoneReceipt = m_rd->SubmitCommandList(version.copyCmdl, m_targetQueue, {}, generateSync);

		m_currVersion = (m_currVersion + 1) % m_maxVersions;

		// Used to sync with other queues (e.g graphics)
		return version.copyDoneReceipt;
	}

}