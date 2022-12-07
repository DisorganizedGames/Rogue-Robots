#pragma once
namespace DOG
{
	enum class SystemType : u8 { Standard , Critical };

	class ISystem
	{
	public:
		ISystem() noexcept = default;
		virtual ~ISystem() noexcept = default;
		virtual void Create() noexcept {}
		virtual void EarlyUpdate() noexcept {}
		virtual void Update() noexcept {}
		virtual void LateUpdate() noexcept {}
#if defined _DEBUG | defined RELWITHDEBUGINFO
		[[nodiscard]] virtual std::string_view GetName() const = 0;
		[[nodiscard]] virtual SystemType GetType() const noexcept = 0;
#endif
	};
}