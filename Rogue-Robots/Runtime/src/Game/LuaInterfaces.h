#include <DOGEngine.h>
#include "Scripting/LuaContext.h"

using namespace DOG;
static class InputInterface
{
public:
	InputInterface() noexcept = default;
	~InputInterface() noexcept = default;

	static void IsLeftPressed(LuaContext* context)
	{
		context->ReturnBoolean(DOG::Mouse::IsButtonPressed(DOG::Button::Left));
	}

	static void IsRightPressed(LuaContext* context)
	{
		context->ReturnBoolean(DOG::Mouse::IsButtonPressed(DOG::Button::Right));
	}

	//Takes a string as argument.
	static void IsKeyPressed(LuaContext* context)
	{
		std::string input = context->GetString();
		context->ReturnBoolean(DOG::Keyboard::IsKeyPressed((DOG::Key)input[0])); //Usch
	}

private:
};