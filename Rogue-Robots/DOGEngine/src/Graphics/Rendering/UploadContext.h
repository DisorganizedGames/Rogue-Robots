#pragma once
#include "../RHI/RenderResourceHandles.h"
#include "../RHI/Types/QueueTypes.h"
#include "../Memory/BumpAllocator.h"

namespace DOG::gfx
{
	class RenderDevice;

	class UploadContext
	{
	public:
		/*
			Initializes a dedicated thread which consumes WaitForEvents pushed by ExecuteCopies to notify
			when a part of the staging buffer is free to be cleared.
		*/
		UploadContext(RenderDevice* rd, u32 stagingSizePerVersion, u8 maxVersions, QueueType targetQueue = QueueType::Graphics);

		/*
			Copies data to local staging and enqueues GPU-GPU copy for later exeuction.
		*/
		void PushUpload(Buffer dst, u32 dstOffset, void* data, u32 dataSize);

		/*
			If target queue is Graphics, no sync receipt is generated.
			If target queue is Copy, sync receipt is generated.
			Compute queue is not allowed.
		*/
		std::optional<SyncReceipt> SubmitCopies();

	private:
		struct Version_Storage
		{
			Buffer staging;
			BumpAllocator vator;
			CommandList copyCmdl;
			std::optional<SyncReceipt> copyDoneReceipt;
		};

	private:
		RenderDevice* m_rd{ nullptr };

		QueueType m_targetQueue{ QueueType::Graphics };

		u8 m_maxVersions{ 0 };
		u8 m_currVersion{ 0 };

		std::vector<Version_Storage> m_storage;

		/*
			maxVersions
			currVersion

			Single Staging Buffer

			maxVersion number of Virtual Bump Allocators
			--> ExecuteCopies moves to next version

			if Copy:
				--> ExecuteCopies pushes wait Event + Version number
				--> Async wait consumes and waits for Event
				--> When event done --> Ator[version number] -> clear()

				Use conditional variables per version (check bool and wait in PushUpload if not done yet)

			Virtual Bump Allocator
		*/

	};

}