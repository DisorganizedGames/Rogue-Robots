#include <DOGEngine.h>
#include "Scripting/LuaContext.h"

using namespace DOG;

class LuaInterface
{
public:
	LuaInterface() noexcept = default;
	virtual ~LuaInterface() noexcept = default;
};

class InputInterface : public LuaInterface
{
public:
	InputInterface() noexcept
	{

	}
	~InputInterface() noexcept
	{
	
	}

	void IsLeftPressed(LuaContext* context)
	{
		context->ReturnBoolean(DOG::Mouse::IsButtonPressed(DOG::Button::Left));
	}

	void IsRightPressed(LuaContext* context)
	{
		context->ReturnBoolean(DOG::Mouse::IsButtonPressed(DOG::Button::Right));
	}

	//Takes a string as argument.
	void IsKeyPressed(LuaContext* context)
	{
		std::string input = context->GetString();
		context->ReturnBoolean(DOG::Keyboard::IsKeyPressed((DOG::Key)input[0])); //Usch
	}
};

class AudioInterface : public LuaInterface
{
public:
	AudioInterface() noexcept
	{

	}
	~AudioInterface() noexcept
	{

	}

	//Takes a string as argument
	void PlaySound(LuaContext* context)
	{
		//TODO
	}
};