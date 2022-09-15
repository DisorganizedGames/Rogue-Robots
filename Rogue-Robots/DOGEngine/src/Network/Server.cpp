#include "Server.h"

namespace DOG
{
	Server::Server()
	{
		m_playerIds.resize(MAX_PLAYER_COUNT);
		
		for (int i = 0; i < MAX_PLAYER_COUNT; i++)
		{
			m_playersServer[i].playerNr = i + 1;
			m_playerIds.at(i) = i;
		}

		//Change denominator to set tick rate
		m_tickrate = 1.0f / 60.0f;


		m_clientsSockets.clear();
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
		if (check != 0)
		{
			throw std::runtime_error("Failed to start WSA on server");
		}

		check = getaddrinfo(NULL, "50005", &addrInput, &addrOutput);
		if (check != 0)
		{
			throw std::runtime_error("Failed to get address info on server");
		}

		//Create socket that listens for new connections
		listenSocket = socket(addrOutput->ai_family, addrOutput->ai_socktype, addrOutput->ai_protocol);
		assert(listenSocket != INVALID_SOCKET);

		check = bind(listenSocket, addrOutput->ai_addr, (int)addrOutput->ai_addrlen);
		assert(check != SOCKET_ERROR);

		check = listen(listenSocket, SOMAXCONN);
		assert(check != SOCKET_ERROR);

		//Thread to handle new connections
		std::thread test = std::thread(&Server::ServerReciveConnections, this, listenSocket);
		test.detach();

		//Thread that runs server
		std::thread test2 = std::thread(&Server::ServerPoll, this);
		test2.detach();

		std::cout << "Server: Server started" << std::endl;

	}

	void Server::ServerReciveConnections(SOCKET listenSocket)
	{
		char* inputSend = new char[sizeof(int)];
		while (true)
		{
			std::cout << "Server: Waiting for connections: " << std::endl;
			SOCKET clientSocket = accept(listenSocket, NULL, NULL);
			//Check if server full
			if (m_playerIds.empty())
			{
				sprintf_s(inputSend, sizeof(int), "%d", -1);
				send(clientSocket, inputSend, sizeof(int), 0);
			}
			else
			{
				{
					std::cout << "Server: Connection Accepted" << std::endl;

					
					WSAPOLLFD m_clientPoll;
					Client::ClientsData input;
					std::cout << "\nServer: Accept a connection from clientSocket: " << clientSocket << ", From player: " << m_playerIds.front() + 1 << std::endl;
					//give connections a player id
					int playerId = m_playerIds.front();
					input.playerNr = playerId + 1;
					m_holdPlayerIds.push_back(playerId);
					sprintf_s(inputSend, sizeof(int), "%d", playerId);
					send(clientSocket, inputSend, sizeof(int), 0);
					m_playerIds.erase(m_playerIds.begin());

					//store client socket
					m_clientPoll.fd = clientSocket;
					m_clientPoll.events = POLLRDNORM;
					m_clientPoll.revents = 0;
					m_clientsSockets.push_back(m_clientPoll);
				}
			}
		}
		delete[] inputSend;
		closesocket(listenSocket);
	}


	float Server::TickTimeLeft(LARGE_INTEGER t, LARGE_INTEGER frequency)
	{
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);

		return float(now.QuadPart - t.QuadPart) / float(frequency.QuadPart);
	}

	void Server::ServerPoll() 
	{
		std::cout << "Server: Started to tick" << std::endl;

		LARGE_INTEGER tickStartTime;
		Client::ClientsData holdClientsData;
		char* clientData = new char[sizeof(Client::ClientsData)];
		char* inputSend = new char[2048];
		LARGE_INTEGER clockFrequency;
		QueryPerformanceFrequency(&clockFrequency);
		
		//sets the minium resolution for ticks
		UINT sleepGranularityMs = 1;
		timeBeginPeriod(sleepGranularityMs);

		do {
			QueryPerformanceCounter(&tickStartTime);

			m_holdSockets = m_clientsSockets;
			
			if (WSAPoll(m_holdSockets.data(), (u32)m_holdSockets.size(), 10) > 0)
			{
				for (int i = 0; i < m_holdSockets.size(); i++)
				{
					if (m_holdSockets[i].revents & POLLERR || m_holdSockets[i].revents & POLLHUP || m_holdSockets[i].revents & POLLNVAL) 
						CloseSocket(i);

					//read in from clients that have send data
					else if (m_holdSockets[i].revents & POLLRDNORM)
					{
						recv(m_holdSockets[i].fd, clientData, sizeof(Client::ClientsData), 0);
						memcpy(&holdClientsData, (void*)clientData, sizeof(Client::ClientsData));
						memcpy(&m_playersServer[holdClientsData.playerNr-1], (void*)clientData, sizeof(Client::ClientsData));
					}
				}
			}

			//Send to all connected clients
			for (int i = 0; i < m_holdSockets.size(); i++)
			{
				memcpy(inputSend, m_playersServer, sizeof(m_playersServer));
				send(m_holdSockets[i].fd, inputSend, sizeof(m_playersServer), 0);
			}
			//wait untill tick is done 
			float timeTakenS = TickTimeLeft(tickStartTime, clockFrequency);

			while (timeTakenS < m_tickrate)
			{
				float timeToWaitMs = (m_tickrate - timeTakenS) * 1000;

				if (timeToWaitMs > 0)
				{
					Sleep((u32)timeToWaitMs);
				}
				timeTakenS = TickTimeLeft(tickStartTime, clockFrequency);
			}
		} while (true);
		std::cout << "Server: server loop closed" << std::endl;
		delete[] clientData;
		delete[] inputSend;
	}

	void Server::CloseSocket(int socketIndex) 
	{
		std::cout << "Server: Closes socket for player" << m_holdPlayerIds.at(socketIndex) + 1 << std::endl;
		m_playerIds.push_back(m_holdPlayerIds.at(socketIndex));
		m_holdPlayerIds.erase(m_holdPlayerIds.begin() + socketIndex);
		m_clientsSockets.erase(m_clientsSockets.begin() + socketIndex);
	}
}
