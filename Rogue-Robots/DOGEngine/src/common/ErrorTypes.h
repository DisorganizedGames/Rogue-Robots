#pragma once

namespace DOG
{
	class FileNotFoundError : std::exception
	{
	public:
		explicit FileNotFoundError(const std::string& filePath) : filePath(filePath) {}

		const char* what() const noexcept {
			return (std::string("File not found: ") + filePath + "\n").c_str();
		}

	private:
		std::string filePath;
	};
}