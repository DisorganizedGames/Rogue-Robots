#include "LuaLayer.h"

using namespace DOG;

f32 frames = 0;
f32 maxFrames = 5'0;

f32 timer = 0;
f32 totalScripts = 1;
std::vector<entity> scriptEntities;

f32 cppTimer = 0;

i32 index = 0;
i32 scriptTypeIndex = 0;

int fake = 0;

int EmptyFunction()
{
	return (int)timer;
}

int CpptoCppFunction()
{
	return EmptyFunction();
}

int ForLoopFunction()
{
	int sum = 0;
	for (int i = 1; i < 25; ++i)
	{
		for (int j = 1; j < 25; ++j)
		{
			sum = sum + i % j;
		}
	}

	return sum;
	/*std::cout << sum << "\n";*/
}

void LuaToCppEmptyFunction(LuaContext*)
{

}

LuaLayer::LuaLayer() noexcept
	: Layer("Lua layer")
{
	LuaMain::GetScriptManager()->SortOrderScripts();
	//Do startup of lua
	LuaMain::GetScriptManager()->RunLuaFile("LuaStartUp.lua");

	LuaMain::GetGlobal()->SetFunction<LuaToCppEmptyFunction>("LuaToCppEmptyFunction");

	LuaMain::GetScriptManager()->RunLuaFile("ProfileGlobalLua.lua");
}

LuaLayer::~LuaLayer()
{
}

void LuaLayer::OnAttach()
{
	for (int i = 0; i < totalScripts; ++i)
	{
		entity e = EntityManager::Get().CreateEntity();
		scriptEntities.push_back(e);
		LuaMain::GetScriptManager()->AddScript(e, "ProfilingLua.lua");
	}
}

void LuaLayer::OnDetach()
{
}

void LuaLayer::OnUpdate()
{
	if (scriptTypeIndex == 3)
		return;

	++frames;

	//MINIPROFILE
	std::chrono::steady_clock::time_point m_startTime;
	std::chrono::steady_clock::time_point m_endTime;

	if (scriptTypeIndex > 0)
	{
		m_startTime = std::chrono::steady_clock::now();

		LuaMain::GetScriptManager()->UpdateScripts(scriptTypeIndex == 2);

		m_endTime = std::chrono::steady_clock::now();
	}
	else
	{
		if (index == 0)
		{
			m_startTime = std::chrono::steady_clock::now();

			for (int i = 0; i < totalScripts; ++i)
			{
				LuaMain::GetGlobal()->CallGlobalFunction("EmptyFunction");
			}

			m_endTime = std::chrono::steady_clock::now();
		}
		else if (index == 1)
		{
			m_startTime = std::chrono::steady_clock::now();

			for (int i = 0; i < totalScripts; ++i)
			{
				LuaMain::GetGlobal()->CallGlobalFunction("CallCppFunction");
			}

			m_endTime = std::chrono::steady_clock::now();
		}
		else if (index == 2)
		{
			m_startTime = std::chrono::steady_clock::now();

			for (int i = 0; i < totalScripts; ++i)
			{
				LuaMain::GetGlobal()->CallGlobalFunction("ForLoopAdd");
			}

			m_endTime = std::chrono::steady_clock::now();
		}
	}

	timer += std::chrono::duration_cast<std::chrono::microseconds>(m_endTime - m_startTime).count();

	if (index == 0)
	{
		m_startTime = std::chrono::steady_clock::now();

		for (int i = 0; i < totalScripts; ++i)
		{
			//For the function not to be optimized away
			fake = EmptyFunction();
		}

		m_endTime = std::chrono::steady_clock::now();
	}
	else if (index == 1)
	{
		m_startTime = std::chrono::steady_clock::now();

		for (int i = 0; i < totalScripts; ++i)
		{
			//For the function not to be optimized away
			fake = CpptoCppFunction();
		}

		m_endTime = std::chrono::steady_clock::now();
	}
	else if (index == 2)
	{
		m_startTime = std::chrono::steady_clock::now();

		for (int i = 0; i < totalScripts; ++i)
		{
			//For the function not to be optimized away
			fake = ForLoopFunction();
		}

		m_endTime = std::chrono::steady_clock::now();
	}

	cppTimer += std::chrono::duration_cast<std::chrono::microseconds>(m_endTime - m_startTime).count();

	if (frames == maxFrames)
	{
		if (index == 0)
			std::cout << "\n\nTesting empty functions" << "\n";
		else if (index == 1)
			std::cout << "\n\nTesting lua to cpp and cpp to cpp functions" << "\n";
		else if (index == 2)
			std::cout << "\n\nTesting for loop" << "\n";

		if (scriptTypeIndex == 0)
			std::cout << "\n\nRun straight in lua file" << "\n";
		else if (scriptTypeIndex == 1)
			std::cout << "\n\nScript with environment" << "\n";
		else if (scriptTypeIndex == 2)
			std::cout << "\n\nScript with environment with coroutine" << "\n";

		f32 avgFrameTime = timer / (f32)maxFrames;
		f32 avgTimeForUpdatingOneScript = avgFrameTime / totalScripts;

		f32 avgCppFrameTime = cppTimer / maxFrames;
		f32 avgCppTimeForOneFunctionCall = avgCppFrameTime / totalScripts;

		std::cout << "Scripts Running OnUpdate: " << totalScripts << "\n";
		std::cout << "Total OnUpdate frame time: " << timer << " microseconds\n";
		std::cout << "Average OnUpdate frame time: " << avgFrameTime << " microseconds\n";
		std::cout << "Average Time For Updating One Script: " << avgTimeForUpdatingOneScript << " microseconds\n";

		std::cout << "Cpp functions calls made per frame: " << totalScripts << "\n";
		std::cout << "Total cpp function call frame time: " << cppTimer << " microseconds\n";
		std::cout << "Average cpp function call frame time: " << avgCppFrameTime << " microseconds\n";
		std::cout << "Average Time For one cpp function call: " << avgCppTimeForOneFunctionCall << " microseconds\n";

		std::cout << fake << "\n\n";

		frames = 0;
		timer = 0;
		cppTimer = 0;

		for (int i = 0; i < scriptEntities.size(); ++i)
		{
			LuaMain::GetScriptManager()->RemoveAllEntityScripts(scriptEntities[i]);
			EntityManager::Get().DestroyEntity(scriptEntities[i]);
		}
		scriptEntities.clear();

		if (totalScripts == 100'00)
		{
			totalScripts = 0.1f;
			++index;
			std::cout << "Change\n";
			if (index == 3)
			{
				++scriptTypeIndex;
				index = 0;
			}
		}
		totalScripts *= 10.0f;
		for (int i = 0; i < totalScripts; ++i)
		{
			entity e = EntityManager::Get().CreateEntity();
			scriptEntities.push_back(e);
			if (index == 0)
				LuaMain::GetScriptManager()->AddScript(e, "ProfilingLua.lua");
			else if (index == 1)
				LuaMain::GetScriptManager()->AddScript(e, "ProfilingLua_CallCPPFunction.lua");
			else if (index == 2)
				LuaMain::GetScriptManager()->AddScript(e, "ProfilingLua_ForLoopAdd.lua");
		}
	}
}

void LuaLayer::OnRender()
{
}

void LuaLayer::OnImGuiRender()
{
}

void LuaLayer::OnEvent(DOG::IEvent& event)
{
}
