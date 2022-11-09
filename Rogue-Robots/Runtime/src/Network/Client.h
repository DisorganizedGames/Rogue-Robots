#pragma once
#include <DOGEngine.h>
#include "Game/GameComponent.h"
#include "Network.h"

class Client
{
public:


	Client();
	~Client();
	INT8 ConnectTcpServer(std::string ipAdress);
	void SendChararrayTcp(char* input, int size);
	int ReceiveCharArrayTcp(char* recivebuffer);
	void SetMulticastAdress(const char* adress);
public:
	void SetUpUdp();
	void SendUdp(PlayerNetworkComponentUdp input);
	UdpReturnData ReceiveUdp();

private:
	u64 m_udpId;
	int m_sendUdpId;
	char* m_hostIp;
	ClientsData m_playersClient[MAX_PLAYER_COUNT];
	SOCKET m_connectSocket;
	char* m_inputSend;
	SOCKET m_udpSendSocket;
	SOCKET m_udpReciveSocket;
	struct sockaddr_in m_hostAddressUdp;
	struct sockaddr_in m_reciveAddressUdp;
	char* m_sendUdpBuffer;
	char* m_reciveUdpBuffer;
	PlayerNetworkComponentUdp m_holdplayersUdp[MAX_PLAYER_COUNT];
	char m_multicastAdress[16];
};