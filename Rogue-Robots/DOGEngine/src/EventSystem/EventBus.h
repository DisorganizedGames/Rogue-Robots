#pragma once
#include "../Core/Application.h"
namespace DOG
{
	class IEvent;
	class EventBus
	{
	public:
		[[nodiscard]] constexpr static EventBus& Get() noexcept { return s_instance; }
		void Dispatch(IEvent&& event) const noexcept;
		void SetMainApplication(Application* application) noexcept;
	private:
		EventBus() noexcept = default;
		~EventBus() noexcept = default;
		DELETE_COPY_MOVE_CONSTRUCTOR(EventBus);
	private:
		static EventBus s_instance;
		Application* m_mainApplication;
	};
}