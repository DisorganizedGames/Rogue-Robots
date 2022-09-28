#pragma once
#include <DOGEngine.h>

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

	void IsLeftPressed(DOG::LuaContext* context);

	void IsRightPressed(DOG::LuaContext* context);

	//Takes a string as argument.
	void IsKeyPressed(DOG::LuaContext* context);
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
	void PlaySound(/*DOG::LuaContext**/);
};

class EntityInterface : public LuaInterface
{
public:
	EntityInterface() noexcept
	{

	}
	~EntityInterface() noexcept
	{

	}

	void CreateEntity(DOG::LuaContext* context);

	void DestroyEntity(DOG::LuaContext* context);

	//Takes an entity-ID as input.
	//Also takes a string that tells what component to add.
	//The user also has to send the appropriate input for that component.
	void AddComponent(DOG::LuaContext* context);

	//Takes an entity-ID as input.
	//Also takes a string that tells what component to change.
	//The user also has to send the appropriate input for that component.
	void ModifyComponent(DOG::LuaContext* context);

	//Takes an entity-ID as input.
	void GetTransformPosData(DOG::LuaContext* context);
	void GetTransformScaleData(DOG::LuaContext* context);

private:
	void AddModel(DOG::LuaContext* context, DOG::entity e);

	void AddTransform(DOG::LuaContext* context, DOG::entity e);

	void AddNetwork(DOG::entity e);

	void ModifyTransform(DOG::LuaContext* context, DOG::entity e);
};

class AssetInterface : public LuaInterface
{
public:
	AssetInterface() noexcept
	{

	}
	~AssetInterface() noexcept
	{

	}

	//Takes a string, path to the model.
	void LoadModel(DOG::LuaContext* context);

private:

};

class PlayerInterface : public LuaInterface
{
public:
	PlayerInterface() noexcept = delete;
	PlayerInterface(DOG::entity player) : m_player{ player }
	{

	}
	~PlayerInterface() noexcept
	{

	}

	void GetForward(DOG::LuaContext* context);
	void GetPosition(DOG::LuaContext* context);
private:
	DOG::entity m_player;
};