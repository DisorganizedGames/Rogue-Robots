#pragma once
#include "../Core/Application.h"
namespace DOG
{
	class IEvent;
	class EventBuss
	{
	public:
		[[nodiscard]] constexpr static EventBuss& Get() noexcept { return s_instance; }
		void Dispatch(IEvent&& event) const noexcept;
		void SetMainApplication(Application* application) noexcept;
	private:
		EventBuss() noexcept = default;
		~EventBuss() noexcept = default;
		DELETE_COPY_MOVE_CONSTRUCTOR(EventBuss);
	private:
		static EventBuss s_instance;
		Application* m_mainApplication;
	};
}