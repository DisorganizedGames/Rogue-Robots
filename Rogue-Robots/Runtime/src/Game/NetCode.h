#pragma once
#include <DOGEngine.h>
#include "..\Game\EntitesTypes.h"
#include  "..\Network\Client.h"
#include  "..\Network\Server.h"
#include "AgentManager/AgentManager.h"
#include "GameComponent.h"

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
	void OnStartup();
	bool Host();
	bool Join(char* input);
	i8 Play();
	u8 GetNrOfPlayers();
	std::string GetIpAdress();
	bool IsLobbyAlive();
	void SetMulticastAdress(const char* adress);
	void SetLobbyStatus(bool lobbyStatus);
	static NetCode* GetNetCode();
	void ResetServer();

private:
	void Receive();
	void ReceiveUdp();
	
	void UpdateSendUdp();
	void AddMatrixUdp(DirectX::XMMATRIX input);

	static NetCode* s_thisNetCode;

	TcpHeader m_inputTcp;
	PlayerNetworkComponentUdp m_playerInputUdp;


	UdpReturnData m_outputUdp;
	
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
	u16 m_bufferSize;
	int m_bufferReceiveSize;
	char m_sendBuffer[SEND_AND_RECIVE_BUFFER_SIZE];
	char* m_receiveBuffer;
	bool m_dataIsReadyToBeReceivedTcp;
	bool m_lobby;
	Server m_serverHost;
	int m_numberOfPackets;
	char m_multicastAdress[16];

	//Tick
	LARGE_INTEGER m_tickStartTime;
	LARGE_INTEGER m_clockFrequency;
	UINT m_sleepGranularityMs;
	
	
};

class DeleteNetworkSync : public DOG::ISystem
{
public:		
	SYSTEM_CLASS(DOG::DeferredDeletionComponent, NetworkId, DOG::TransformComponent);
	ON_LATE_UPDATE_ID(DOG::DeferredDeletionComponent, NetworkId, DOG::TransformComponent);
	void OnLateUpdate(DOG::entity e, DOG::DeferredDeletionComponent& deleteC, NetworkId& netId, DOG::TransformComponent& transC);
};