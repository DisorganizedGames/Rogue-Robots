#pragma once
namespace DOG
{
	class IEvent;
	class Layer
	{
	public:
		Layer(const std::string& name) noexcept : m_name{ name } {};
		virtual ~Layer() noexcept = default;
		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnUpdate() {}
		virtual void OnRender() {}
		virtual void OnImGuiRender() {}
		virtual void OnEvent(IEvent&) {}
		[[nodiscard]] constexpr const std::string& GetName() const noexcept { return m_name; }
	private:
		std::string m_name;
	};
}
