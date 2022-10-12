#pragma once
#include "../Core/Application.h"

namespace DOG
{
	constexpr int MAX_PLAYER_COUNT = 4;
	constexpr int SEND_AND_RECIVE_BUFFER_SIZE = 2048;
	constexpr const char* PORTNUMBER_OUT = "50005";
	constexpr const char* PORTNUMBER_IN = "50004";
	constexpr int PORTNUMBER_OUT_INT = 50006;
	constexpr int PORTNUMBER_IN_INT = 50004;
	constexpr const char* MULTICAST_ADRESS = "239.255.255.0";
	class Client
	{
	public:
		struct NetworkComponent
		{
			UINT8 playerId;
			unsigned short componentId;
			DirectX::XMMATRIX componentMatrix;
		};
		struct PlayerNetworkComponent
		{
			int playerId = 0;
			int udpId = 0;
			DirectX::XMMATRIX matrix = {};
			bool shoot = false;
			bool jump = false;
			bool activateActiveItem = false;
			bool switchComp = false;


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
		struct ClientsData
		{
			int playerId = 0;
			DirectX::XMMATRIX matrix = {};
		};
		Client();
		~Client();
		int ConnectTcpServer(std::string ipAdress);
		void SendTcp(ClientsData input);
		ClientsData* ReciveTcp();
		ClientsData* SendandReciveTcp(ClientsData input);
		void SetUpUdp();
		void SendUdp(PlayerNetworkComponent input);
		UdpReturnData ReciveUdp();
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
}