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

class EntityInterface : public LuaInterface
{
public:
	EntityInterface() noexcept
	{

	}
	~EntityInterface() noexcept
	{

	}

	void CreateEntity(LuaContext* context)
	{
		context->ReturnInteger(EntityManager::Get().CreateEntity());
	}

	void DestroyEntity(LuaContext* context)
	{
		EntityManager::Get().DestroyEntity(context->GetInteger());
	}

	//Takes an entity-ID as input.
	//Also takes a string that tells what component to add.
	//The user also has to send the appropriate input for that component.
	void AddComponent(LuaContext* context)
	{
		LuaW::s_luaW.PrintStack();
		entity e = context->GetInteger();
		std::string compType = context->GetString();

		if (compType == "Model")
		{
			AddModel(context, e);
		}
		else if (compType == "Transform")
		{
			AddTransform(context, e);
		}
		//Add more component types here.
	}

	//Takes an entity-ID as input.
	//Also takes a string that tells what component to change.
	//The user also has to send the appropriate input for that component.
	void ModifyComponent(LuaContext* context)
	{
		entity e = context->GetInteger();
		std::string compType = context->GetString();

		if (compType == "Transform")
		{
			ModifyTransform(context, e);
		}
		//Add more component types here.
	}

	//Takes an entity-ID as input.
	void GetTransformPosData(LuaContext* context)
	{
		entity e = context->GetInteger();
		TransformComponent& transform = EntityManager::Get().GetComponent<TransformComponent>(e);

		DirectX::SimpleMath::Vector3 pos = transform.worldMatrix.Translation();

		LuaTable t(&LuaW::s_luaW);
		t.AddFloatToTable("x", pos.x);
		t.AddFloatToTable("y", pos.y);
		t.AddFloatToTable("z", pos.z);
		context->ReturnTable(t);
	}

private:
	void AddModel(LuaContext* context, entity e)
	{
		EntityManager::Get().AddComponent<ModelComponent>(e, std::stoull(context->GetString()));
	}

	void AddTransform(LuaContext* context, entity e)
	{
		LuaW::s_luaW.PrintStack();
		LuaTable pos = context->GetTable();
		LuaTable rot = context->GetTable();
		LuaTable scale = context->GetTable();
		LuaW::s_luaW.PrintStack();

		EntityManager::Get().AddComponent<TransformComponent>(e)
			.SetPosition({ pos.GetFloatFromTable("x"), pos.GetFloatFromTable("y") , pos.GetFloatFromTable("z") })
			.SetRotation({ rot.GetFloatFromTable("x"), rot.GetFloatFromTable("y") , rot.GetFloatFromTable("z") })
			.SetScale({ scale.GetFloatFromTable("x"), scale.GetFloatFromTable("y"), scale.GetFloatFromTable("z") });
	}

	void ModifyTransform(LuaContext* context, entity e)
	{
		TransformComponent& transform = EntityManager::Get().GetComponent<TransformComponent>(e);
		LuaTable t = context->GetTable();
		switch (context->GetInteger())
		{
		case 1:
			transform.SetPosition({ t.GetFloatFromTable("x"), t.GetFloatFromTable("y"), t.GetFloatFromTable("z") });
		case 2:
			transform.SetRotation({ t.GetFloatFromTable("x"), t.GetFloatFromTable("y") , t.GetFloatFromTable("z") });
		case 3:
			transform.SetScale({ t.GetFloatFromTable("x"), t.GetFloatFromTable("y") , t.GetFloatFromTable("z") });
		}
	}
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
	void LoadModel(LuaContext* context)
	{
		//Send string? Cast to double? Change asset manager to use 32bit?
		context->ReturnString(std::to_string(AssetManager::Get().LoadModelAsset(context->GetString())));
	}

private:

};