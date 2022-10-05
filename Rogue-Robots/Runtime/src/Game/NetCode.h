#pragma once
#include <DOGEngine.h>
#include "../DOGEngine/src/Network/Server.h"
#include "../DOGEngine/src/Network/Client.h"
#include "MainPlayer.h"
using namespace DOG;
enum PlayerActions
{
	Shoot = 0x01,
	Jump = 0x02,
	Temp1 = 0x04,
	Temp2 = 0x08,
	Temp3 = 0x10,
	Temp4 = 0x20
};
class NetCode
{
public:
	NetCode();
	~NetCode();
	
	void OnUpdate();
	void AddPlayersId(std::vector<entity> playersId);

	void AddMatrixTcp(DirectX::XMMATRIX input);

	void AddMatrixUdp(DirectX::XMMATRIX input);


private:
	void Recive();
	void ReciveUdp();
	DOG::Client::ClientsData m_inputTcp;
	DOG::Client::PlayerNetworkComponent m_playerInputUdp;

	DOG::Client::ClientsData* m_outputTcp;
	DOG::Client::UdpReturnData m_outputUdp;

	std::atomic_bool m_active;
	std::atomic_bool m_startUp;
	std::atomic_bool m_hardSyncTcp;
	uint16_t m_tick = 0;
	std::atomic_bool m_netCodeAlive;
	std::thread m_thread;
	std::thread m_threadUdp;
	std::vector<entity> m_playersId;
	std::string m_inputString;
	DOG::Client m_client;
	std::mutex m_mut;
};