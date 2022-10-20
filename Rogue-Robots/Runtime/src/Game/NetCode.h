#pragma once
#include <DOGEngine.h>
#include  "..\Network\Client.h"
#include  "..\Network\Server.h"
#include "..\Game\MainPlayer.h"

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



private:
	void Recive();
	void ReciveUdp();


	void AddMatrixUdp(DirectX::XMMATRIX input);

	
	Client::ClientsData m_inputTcp;
	Client::PlayerNetworkComponent m_playerInputUdp;

	Client::ClientsData* m_outputTcp;
	Client::UdpReturnData m_outputUdp;
	
	std::atomic_bool m_active;
	std::atomic_bool m_startUp;
	std::atomic_bool m_hardSyncTcp;
	uint16_t m_tick = 0;
	std::atomic_bool m_netCodeAlive;
	std::thread m_thread;
	std::thread m_threadUdp;
	std::vector<DOG::entity> m_playersId;
	std::string m_inputString;
	Client m_client;
	std::mutex m_mut;
	int m_bufferSize;
	int m_bufferReciveSize;
	char m_sendBuffer[SEND_AND_RECIVE_BUFFER_SIZE];
	char* m_reciveBuffer;
	bool m_dataIsReadyToBeSentTcp;
	bool m_dataIsReadyToBeRecivedTcp;
};