#pragma once
namespace DOG
{
	struct ApplicationSpecification
	{

	};

	class Application
	{
	public:
		explicit Application(const ApplicationSpecification& spec) noexcept;
		virtual ~Application() noexcept = default;
	private:
		ApplicationSpecification m_Specification;
	};
}

[[nodiscard]] const std::unique_ptr<DOG::Application> CreateApplication() noexcept;