#pragma once
#include <DOGEngine.h>
#include "../DOGEngine/src/Network/Server.h"
#include "../DOGEngine/src/Network/Client.h"
#include "MainPlayer.h"
using namespace DOG;
class NetCode
{
public:
	NetCode();
	~NetCode();
	
	void OnUpdate(std::shared_ptr<MainPlayer> player);
	void AddPlayersId(std::vector<entity> playersId);
	void AddMatrix(DirectX::XMMATRIX input);
	void AddPosition(DirectX::SimpleMath::Vector3 input);
	void AddRotation(DirectX::SimpleMath::Vector3 input);
	
	DOG::Client::ClientsData m_input;
	DOG::Client::ClientsData* m_Output;
	bool m_active;
	bool m_startUp;
private:
	void Recive();
	std::atomic_bool m_netCodeAlive;
	std::thread m_thread;
	std::vector<entity> m_playersId;
	std::string m_inputString;
	DOG::Client m_client;
	std::mutex m_mut;
};