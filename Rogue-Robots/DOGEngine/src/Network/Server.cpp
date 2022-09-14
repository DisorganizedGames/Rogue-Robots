#include "Server.h"

namespace DOG
{
	Server::Server()
	{
		m_serverAlive = TRUE;
		m_playerIds.resize(MAX_PLAYER_COUNT);
		
		for (int i = 0; i < MAX_PLAYER_COUNT; i++)
		{
			m_playersServer[i].playerId = i;
			m_playersServer[i].position = DirectX::XMVectorSet(0, (float)i*2.0f+1.0f, 0, 0);
			m_playersServer[i].rotation = DirectX::XMVectorSet(0, 0, 0, 0);
			m_playerIds.at(i) = i;
			
		}

		//Change denominator to set tick rate
		m_tickrate = 1.0f / 120.0f;


		m_clientsSockets.clear();
	}

	Server::~Server()
	{
		m_serverAlive = FALSE;
		m_serverLoop.join();
		m_reciveConnections.join();
		for (int socketIndex = 0; socketIndex < m_holdPlayerIds.size(); socketIndex++)
		{
			std::cout << "Server: Closes socket for player" << m_holdPlayerIds.at(socketIndex) + 1 << std::endl;
			m_playerIds.push_back(m_holdPlayerIds.at(socketIndex));
			m_holdPlayerIds.erase(m_holdPlayerIds.begin() + socketIndex);
			m_clientsSockets.erase(m_clientsSockets.begin() + socketIndex);
		}
	}

	bool Server::StartTcpServer()
	{
		std::cout << "Server: Starting server..." << std::endl;
		int check;
		unsigned long setUnblocking = 1;
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
			std::cout << "Server: Failed to start WSA on server, ErrorCode: " << check << std::endl;
			return FALSE;
		}

		check = getaddrinfo(NULL, "50005", &addrInput, &addrOutput);
		if (check != 0)
		{
			std::cout << "Server: Failed to getaddrinfo on server, ErrorCode: " << check << std::endl;
			return FALSE;
		}
		
		//Create socket that listens for new connections
		listenSocket = socket(addrOutput->ai_family, addrOutput->ai_socktype, addrOutput->ai_protocol);
		if (listenSocket == INVALID_SOCKET)
		{
			std::cout << "Server: Failed to create listensocket on server, ErrorCode: " << WSAGetLastError() << std::endl;
			return FALSE;
		}

		
		check = ioctlsocket(listenSocket, FIONBIO, (unsigned long*) &setUnblocking);
		if (check == SOCKET_ERROR)
		{
			std::cout << "Server: Failed to set listensocket to unblocking on server, ErrorCode: " << WSAGetLastError() << std::endl;
			return FALSE;
		}

		check = bind(listenSocket, addrOutput->ai_addr, (int)addrOutput->ai_addrlen);
		if (check == SOCKET_ERROR)
		{
			std::cout << "Server: Failed to bind listenSocket on server, ErrorCode: " << WSAGetLastError() << std::endl;
			return FALSE;
		}

		check = listen(listenSocket, SOMAXCONN);
		if (check == SOCKET_ERROR)
		{
			std::cout << "Server: Failed to SOMAXCONN on server, ErrorCode: " << WSAGetLastError() << std::endl;
			return FALSE;
		}

		//Thread to handle new connections
		m_reciveConnections = std::thread(&Server::ServerReciveConnectionsTCP, this, listenSocket);

		//Thread that runs server
		m_serverLoop = std::thread(&Server::ServerPollTCP, this);

		std::cout << "Server: Server started" << std::endl;
		return TRUE;
	}

	void Server::ServerReciveConnectionsTCP(SOCKET listenSocket)
	{
		char* inputSend = new char[sizeof(int)];
		while (m_serverAlive)
		{
			SOCKET clientSocket = accept(listenSocket, NULL, NULL);
			//Check if server full
			if (clientSocket != INVALID_SOCKET)
			{
				if (m_playerIds.empty())
				{
					sprintf_s(inputSend, sizeof(int), "%d", -1);
					send(clientSocket, inputSend, sizeof(int), 0);
				}
				else
				{
					{
						bool turn = true;
						std::cout << "Server: Connection Accepted" << std::endl;
						setsockopt(clientSocket, SOL_SOCKET, TCP_NODELAY, (char*)&turn, sizeof(bool));
						WSAPOLLFD m_clientPoll;
						Client::ClientsData input;
						std::cout << "\nServer: Accept a connection from clientSocket: " << clientSocket << ", From player: " << m_playerIds.front() + 1 << std::endl;
						//give connections a player id
						int playerId = m_playerIds.front();
						input.playerId = playerId;
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
		}
		delete[] inputSend;
		closesocket(listenSocket);
	}


	float Server::TickTimeLeftTCP(LARGE_INTEGER t, LARGE_INTEGER frequency)
	{
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);

		return float(now.QuadPart - t.QuadPart) / float(frequency.QuadPart);
	}

	void Server::ServerPollTCP() 
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
			
			if (WSAPoll(m_holdSockets.data(), (u32)m_holdSockets.size(), 1) > 0)
			{
				for (int i = 0; i < m_holdSockets.size(); i++)
				{
					if (m_holdSockets[i].revents & POLLERR || m_holdSockets[i].revents & POLLHUP || m_holdSockets[i].revents & POLLNVAL) 
						CloseSocketTCP(i);

					//read in from clients that have send data
					else if (m_holdSockets[i].revents & POLLRDNORM)
					{
						recv(m_holdSockets[i].fd, clientData, sizeof(Client::ClientsData), 0);
						memcpy(&holdClientsData, (void*)clientData, sizeof(Client::ClientsData));
						memcpy(&m_playersServer[holdClientsData.playerId], (void*)clientData, sizeof(Client::ClientsData));
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
			float timeTakenS = TickTimeLeftTCP(tickStartTime, clockFrequency);

			while (timeTakenS < m_tickrate)
			{
				float timeToWaitMs = (m_tickrate - timeTakenS) * 1000;

				if (timeToWaitMs > 0)
				{
					Sleep((u32)timeToWaitMs);
				}
				timeTakenS = TickTimeLeftTCP(tickStartTime, clockFrequency);
			}
		} while (m_serverAlive);
		std::cout << "Server: server loop closed" << std::endl;
		delete[] clientData;
		delete[] inputSend;
	}

	void Server::CloseSocketTCP(int socketIndex) 
	{
		std::cout << "Server: Closes socket for player" << m_holdPlayerIds.at(socketIndex) + 1 << std::endl;
		m_playerIds.push_back(m_holdPlayerIds.at(socketIndex));
		m_holdPlayerIds.erase(m_holdPlayerIds.begin() + socketIndex);
		m_clientsSockets.erase(m_clientsSockets.begin() + socketIndex);
	}

	std::string Server::GetIpAddress()
	{
		int check;
		char hold[128];
		std::string ip = "";
		struct addrinfo* result , *linked;
	
		check = gethostname(hold, sizeof(hold));
		if (check == SOCKET_ERROR)
		{
			std::cout << "GetIpAddress: gethostname failed, error code: " << WSAGetLastError() << std::endl;
			return ip;
		}
		check = getaddrinfo(hold, NULL, NULL, &result);
		if (check == SOCKET_ERROR)
		{
			std::cout << "GetIpAddress: getaddrinfo failed, error code: " << WSAGetLastError() << std::endl;
			return ip;
		}
		linked = result;
		while (linked)
		{
			if (linked->ai_family == AF_INET)
				inet_ntop(linked->ai_family, &((struct sockaddr_in*)linked->ai_addr)->sin_addr, hold, 100);
			linked = linked->ai_next;
		}
		for (int i = 0; i < sizeof(hold); i++)
		{
			if (hold[i] == '\0')
				break;
			ip.push_back(hold[i]);

		}
		freeaddrinfo(result);
		return ip;
	}
}

