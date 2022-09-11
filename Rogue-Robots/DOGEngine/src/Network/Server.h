#pragma once
namespace DOG
{
	constexpr int m_nrOfPlayers = 2;
	class Server
	{
	public:
		struct ClientsData
		{
			int player_nr = 0;
			bool w = false;
			bool a = false;
			bool s = false;
			bool d = false;
			char inputs[32];
			int input_lentgh;
		};
		Server();
		~Server();

		void StartTcpServer();
	private:
		void ServerReciveConnections(SOCKET listenSocket);
		void ServerPoll();
		void CloseSocket(int socketIndex);
		float TickTimeLeft(LARGE_INTEGER t, LARGE_INTEGER frequency);

		float m_tickrate;

		ClientsData m_playersServer[m_nrOfPlayers];
		std::vector<int>	m_playerIds;
		std::vector<WSAPOLLFD> m_clientsSockets;
		std::vector<WSAPOLLFD> m_holdSockets;
	};
}