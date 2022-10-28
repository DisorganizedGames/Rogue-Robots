#pragma once
#include <DOGEngine.h>
#include "Game/GameComponent.h"

	constexpr int MAX_PLAYER_COUNT = 4;
	constexpr int SEND_AND_RECIVE_BUFFER_SIZE = 8192;
	constexpr const char* PORTNUMBER_OUT = "50005";
	constexpr const char* PORTNUMBER_IN  = "50004";
	constexpr int PORTNUMBER_OUT_INT = 50006;
	constexpr int PORTNUMBER_IN_INT = 50004;
	constexpr const char* MULTICAST_ADRESS = "239.255.255.0";
	class Client
	{
	public:
		struct PlayerNetworkComponent
		{
			int playerId = 0;
			int udpId = 0;
			DirectX::XMMATRIX matrix = {};
			InputController action;

		};
		struct UdpData
		{
			int nrOfEntites;
			int udpId;

		};
		struct UdpReturnData
		{
			PlayerNetworkComponent m_holdplayersUdp[MAX_PLAYER_COUNT];
		};
		struct TcpHeader
		{
			i8 playerId = 0;
			i8 nrOfPlayersConnected = 0;
			u16 sizeOfPayload = 0;
			u16 nrOfNetTransform = 0;
			u16 nrOfNetStats = 0;
			u16 nrOfCreateAndDestroy = 0;
			bool lobbyAlive = true;
			
		};
		struct ClientsData
		{
			i8 playerId = 0;
			u8 nrOfPlayersConnected = 0;
			u16 sizeOfPayload = 0;
			u16 nrOfNetTransform = 0;
			u16 nrOfNetStats = 0;
			u16 nrOfCreateAndDestroy = 0;
			bool lobbyAlive = false;
		};

		Client();
		~Client();
		INT8 ConnectTcpServer(std::string ipAdress);
		void SendChararrayTcp(char* input, int size);
		u8 ReceiveCharArrayTcp(char* recivebuffer);
	public:
		void SetUpUdp();
		void SendUdp(PlayerNetworkComponent input);
		UdpReturnData ReceiveUdp();
		UdpReturnData SendandReciveUdp(PlayerNetworkComponent input);

	private:
		int m_udpId;
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
		PlayerNetworkComponent m_holdplayersUdp[MAX_PLAYER_COUNT];
	};
