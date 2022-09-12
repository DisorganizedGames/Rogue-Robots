#pragma once

namespace DOG
{
	class FileNotFoundError : public std::exception
	{
	public:
		explicit FileNotFoundError(const std::string& filePath) : filePath(filePath) {}

		const char* what() noexcept
		{
			outString = std::string("File not found: ") + filePath + "\n";
			return outString.c_str();
		}

	private:
		std::string filePath;
		std::string outString;
	};
	
	class NoVoiceAvailableError : public std::exception
	{
	public:
		explicit NoVoiceAvailableError() = default;

		const char* what() const noexcept
		{
			return "No voice available to play audio on";
		}
	};
}