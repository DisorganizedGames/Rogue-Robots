#include "Server.h"

namespace DOG
{
	std::barrier sync(m_nrOfPlayers);
	Server::Server()
	{
		m_playerIds.resize(m_nrOfPlayers);
		m_clientThreads.resize(m_nrOfPlayers);
		for (int i = 0; i < m_nrOfPlayers; i++)
		{
			m_playersServer[i].player_nr = i + 1;
			m_clientThreads.at(i) = std::thread(&Server::ClientThreadIdle, this);
			m_playerIds.at(i) = i;
		}

		//Change denominator to set tick rate
		m_tickrate = 1.0f / 60.0f;

	}

	Server::~Server()
	{

	}

	void Server::StartTcpServer()
	{
		std::cout << "Server: Starting server..." << std::endl;
		int check;
		WSADATA socketStart;
		SOCKET listenSocket = INVALID_SOCKET;
		addrinfo* addrOutput = NULL, addrInput;

		ZeroMemory(&addrInput, sizeof(addrInput));
		addrInput.ai_family = AF_INET;
		addrInput.ai_socktype = SOCK_STREAM;
		addrInput.ai_protocol = IPPROTO_TCP;
		addrInput.ai_flags = AI_PASSIVE;

		check = WSAStartup(0x202, &socketStart);
		assert(check == 0);

		check = getaddrinfo(NULL, "50005", &addrInput, &addrOutput);
		assert(check == 0);

		//Create socket that listens for new connections
		listenSocket = socket(addrOutput->ai_family, addrOutput->ai_socktype, addrOutput->ai_protocol);
		assert(listenSocket != INVALID_SOCKET);

		check = bind(listenSocket, addrOutput->ai_addr, (int)addrOutput->ai_addrlen);
		assert(check != SOCKET_ERROR);

		check = listen(listenSocket, SOMAXCONN);
		assert(check != SOCKET_ERROR);

		//Create a new thread for each connecting client
		std::thread test = std::thread(&Server::ServerLoop, this, listenSocket);
		test.detach();
		std::cout << "Server: Server started" << std::endl;

	}

	void Server::ServerLoop(SOCKET listenSocket)
	{
		//std::function<void()> job = &Network::ClientLoop;

		char* inputSend = new char[sizeof(int)];
		//SOCKET clientSocket = INVALID_SOCKET;

		while (true)
		{
			std::cout << "Server: Before accepting a connection: " << std::endl;
			SOCKET clientSocket = accept(listenSocket, NULL, NULL);
			if (m_playerIds.empty())
			{
				sprintf(inputSend, "%d", -1);
				send(clientSocket, inputSend, sizeof(int), 0);
			}
			else
			{


				std::cout << "Server: Thread Accept:" << std::this_thread::get_id() << std::endl;

				std::unique_lock<std::mutex> lock(m_clientMutex);
				std::cout << "\nServer: Accept a connection from clientSocket: " << clientSocket << ", From player: " << m_playerIds.front() + 1 << std::endl;
				int playerId = m_playerIds.front();
				std::function<void()> job = [this, clientSocket, playerId]()
				{
					Server::ClientLoop(clientSocket, playerId);
				};
				m_playerIds.erase(m_playerIds.begin());
				m_clientThreadsQueue.push(job);

			}
			m_clientMutexCondition.notify_one();
		}

		closesocket(listenSocket);
	}

	void Server::ClientThreadIdle() {

		while (true)
		{
			std::function<void()> job;
			{

				std::unique_lock<std::mutex> lock(m_clientMutex);
				m_clientMutexCondition.wait(lock, [this] {return !m_clientThreadsQueue.empty();  });
				std::cout << "Server: From thread: " << std::this_thread::get_id() << std::endl;
				job = m_clientThreadsQueue.front();
				m_clientThreadsQueue.pop();
			}
			job();
		}
		std::cout << "Server: no more threads" << std::endl;
	}

	float Server::TickTimeLeft(LARGE_INTEGER t, LARGE_INTEGER frequency)
	{
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);

		return float(now.QuadPart - t.QuadPart) / float(frequency.QuadPart);
	}

	void Server::ClientLoop(SOCKET clientSocket, int playerIndex)
	{
		std::cout << "Server: connected to ClientLoop" << std::endl;
		char optval[1];
		socklen_t optlen = sizeof(optval);
		BOOL turn = true;


		setsockopt(clientSocket, SOL_SOCKET, TCP_NODELAY, (char*)&turn, sizeof(bool));

		LARGE_INTEGER tickStartTime;
		LARGE_INTEGER timeNow;
		const UINT sleepGranularityMs = 1;
		char* clientData = new char[sizeof(ClientsData)];
		char* inputSend = new char[sizeof(m_playersServer)];

		int status;


		LARGE_INTEGER clockFrequency;
		QueryPerformanceFrequency(&clockFrequency);

		//give client player number
		sprintf(inputSend, "%d", m_playersServer[playerIndex].player_nr);
		send(clientSocket, inputSend, sizeof(int), 0);

		std::cout << "Server: Lobby:\nServer: Waiting for " << (m_playerIds.size()) << " more players to connect..." << std::endl;

		while (m_nrOfPlayers - (m_playerIds.size()) < m_nrOfPlayers)
		{
			status = send(clientSocket, inputSend, sizeof(int), 0);
			continue;
		}
		sprintf(inputSend, "%d", -2);
		send(clientSocket, inputSend, sizeof(int), 0);
		std::cout << "Server: All players connected, Starting " << std::endl;

		do {

			QueryPerformanceCounter(&tickStartTime);
			status = recv(clientSocket, clientData, sizeof(ClientsData), 0);

			//get client input
			memcpy(&m_playersServer[playerIndex], (void*)clientData, sizeof(ClientsData));

			sync.arrive_and_wait();
			//set clients input
			memcpy(inputSend, m_playersServer, sizeof(m_playersServer));

			//sync
			//m_syncCounter--;
			//while (m_syncCounter > 0) {
			//	continue;
			//}
			//m_syncCounter++;
			// 
			//send all clients input
			status = send(clientSocket, inputSend, sizeof(m_playersServer), 0);

			//wait untill tick is done 
			float timeTakenS = TickTimeLeft(tickStartTime, clockFrequency);

			while (timeTakenS < m_tickrate)
			{
				float timeToWaitMs = (m_tickrate - timeTakenS) * 1000;

				if (timeToWaitMs > 0)
				{

					Sleep(timeToWaitMs);
				}
				timeTakenS = TickTimeLeft(tickStartTime, clockFrequency);
			}
		} while (status > 0);
		std::cout << "Server: server thread closes..." << std::endl;
		m_playerIds.push_back(playerIndex);
		if (closesocket(clientSocket) == SOCKET_ERROR)
			std::cout << "Server: AAaAAA Nåt är jävligt fel" << WSAGetLastError() << std::endl;
	}
}
