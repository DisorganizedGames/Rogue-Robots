#pragma once
#include "../Core/Application.h"

namespace DOG
{
	constexpr int MAX_PLAYER_COUNT = 4;

	class Client
	{
	public:
		struct ClientsData
		{
			int playerId = 0;
			char inputs[256] = {0};
			int inputLength = 0;
			DirectX::XMMATRIX matrix;
			DirectX::SimpleMath::Vector3 position;
			DirectX::SimpleMath::Vector3 rotation;
		};
		Client();
		~Client();
		int ConnectTcpServer(std::string ipAdress);
		void SendTcp(ClientsData input);
		ClientsData* ReciveTcp();
		ClientsData* SendandReciveTcp(ClientsData input);
		ClientsData CleanClientsData(ClientsData player);
	private:
		ClientsData m_playersClient[MAX_PLAYER_COUNT];
		SOCKET m_connectSocket;
		char* m_inputSend;
	};
}