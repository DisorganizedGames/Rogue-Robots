#include "Client.h"
using namespace DOG;
	Client::Client()
	{
		ClientsData test;
		m_connectSocket = INVALID_SOCKET;
		m_inputSend = new char[sizeof(ClientsData)];
		m_hostIp = new char[64];
		m_sendUdpBuffer = new char[sizeof(Client::PlayerNetworkComponent)];
		m_reciveUdpBuffer = new char[SEND_AND_RECIVE_BUFFER_SIZE];
		m_udpId = 0;
		m_sendUdpId = 0;
		ZeroMemory(&m_hostAddressUdp, sizeof(m_hostAddressUdp));
		ZeroMemory(&m_reciveAddressUdp, sizeof(m_reciveAddressUdp));
	}

	Client::~Client()
	{
		delete[] m_inputSend;
		delete[] m_hostIp;
		delete[] m_sendUdpBuffer;
		delete[] m_reciveUdpBuffer;
	}

	int Client::ConnectTcpServer(std::string ipAdress)
	{
		WSADATA socketStart;
		addrinfo client, * addrOutput = NULL, * ptr = NULL;
		BOOL turn = TRUE;
		char* inputSend = new char[sizeof(int)];
		int check, returnValue;

		strcpy_s(m_hostIp, sizeof(ipAdress), ipAdress.c_str());

		ZeroMemory(&client, sizeof(client));
		client.ai_family = AF_INET;
		client.ai_socktype = SOCK_STREAM;
		client.ai_protocol = IPPROTO_TCP;


		check = WSAStartup(0x202, &socketStart);
		if (check != 0)
		{
			std::cout << "Client: Failed to start WSA on client, ErrorCode: " << check << std::endl;
			return -1;
		}

		check = getaddrinfo(m_hostIp, "50005", &client, &addrOutput);
		if (check != 0)
		{
			std::cout << "Client: Failed to get address on client, ErrorCode: " << check << std::endl;
			return -1;
		}

		//connect to server
		for (ptr = addrOutput; ptr != NULL; ptr = ptr->ai_next)
		{
			m_connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
			if (m_connectSocket == INVALID_SOCKET)
			{
				std::cout << "Client: Failed to create connectSocket on client, ErrorCode: " << WSAGetLastError() << std::endl;
				return -1;
			}

			check = connect(m_connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
			if (check == SOCKET_ERROR) {
				closesocket(m_connectSocket);
				m_connectSocket = INVALID_SOCKET;
				continue;
			}
			break;
		}
		std::cout << "CLient: Connected to server" << std::endl;

		freeaddrinfo(addrOutput);
		if (m_connectSocket == INVALID_SOCKET)
		{
			std::cout << "Client: Failed to connect connectSocket on client, ErrorCode: " << WSAGetLastError() << std::endl;
			return -1;
		}

		//set socket to tcp_nodelay
		check = setsockopt(m_connectSocket, SOL_SOCKET, TCP_NODELAY, (char*)&turn, sizeof(bool));
		if (check == SOCKET_ERROR)
		{
			std::cout << "Client: Failed to set socket to tcp_nodelay on client, ErrorCode: " << WSAGetLastError() << std::endl;
			return -1;
		}

		//get player number
		check = recv(m_connectSocket, inputSend, sizeof(int), 0);
		returnValue = atoi(inputSend);

		delete[] inputSend;
		SetUpUdp();
		if (returnValue == -1)
		{
			std::cout << "\nCLient: Server Full: " << returnValue << std::endl;
			return -1;
		}
		else
		{
			std::cout << "\nCLient: Player nr: " << returnValue +1 << std::endl;
			return returnValue;
		}
	}

	void Client::SendTcp(ClientsData input)
	{
		memcpy(m_inputSend, &input, sizeof(ClientsData));
		send(m_connectSocket, m_inputSend, sizeof(ClientsData), 0);
		return;
	}

	struct Client::ClientsData* Client::ReciveTcp()
	{
		char recvbuf[2048];
		if (recv(m_connectSocket, recvbuf, sizeof(recvbuf), 0) > -1)
			memcpy(m_playersClient, recvbuf, sizeof(m_playersClient));
		else
			m_playersClient[0].playerId = -1;
		return m_playersClient;
	}

	struct Client::ClientsData* Client::SendandReciveTcp(ClientsData input) 
	{
		char recvbuf[SEND_AND_RECIVE_BUFFER_SIZE];
		memcpy(m_inputSend, &input, sizeof(ClientsData));
		send(m_connectSocket, m_inputSend, sizeof(ClientsData), 0);
		if(recv(m_connectSocket, recvbuf, sizeof(recvbuf), 0) > -1)
			memcpy(m_playersClient, recvbuf, sizeof(m_playersClient));
		return m_playersClient;
	}

	void Client::SetUpUdp()
	{
		int check = 0;
		DWORD ttl = 5020;
		m_udpSendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (m_udpSendSocket == INVALID_SOCKET)
		{
			std::cout << "Client: Failed to create udpSocket on client, ErrorCode: " << WSAGetLastError() << std::endl;
		}

		
		m_hostAddressUdp.sin_family = AF_INET;
		inet_pton(AF_INET, "239.255.255.0", &m_hostAddressUdp.sin_addr.s_addr); //inet_addr("239.255.255.0");
		m_hostAddressUdp.sin_port = htons(50004);

		//recive
		bool turn = TRUE;
		struct ip_mreq setMulticast;
		m_udpReciveSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (m_udpReciveSocket == INVALID_SOCKET)
		{
			std::cout << "Client: Failed to create udpSocket on client, ErrorCode: " << WSAGetLastError() << std::endl;
		}

		check = setsockopt(m_udpReciveSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&turn, sizeof(bool));
		if (check == SOCKET_ERROR)
		{
			std::cout << "Client: Failed to set udpsocket to reusabale adress to unblocking on server, ErrorCode: " << WSAGetLastError() << std::endl;
		}

		
		m_reciveAddressUdp.sin_family = AF_INET;
		m_reciveAddressUdp.sin_addr.s_addr = htonl(INADDR_ANY);//inet_addr("239.255.255.0"); //htonl(INADDR_ANY);
		m_reciveAddressUdp.sin_port =  htons(50006);

		check = bind(m_udpReciveSocket, (struct sockaddr*)&m_reciveAddressUdp, sizeof(m_reciveAddressUdp));
		if (check == SOCKET_ERROR)
		{
			std::cout << "Server: Failed to bind udpsocket on server, ErrorCode: " << WSAGetLastError() << std::endl;
		}

		inet_pton(AF_INET, "239.255.255.0", &setMulticast.imr_multiaddr.S_un.S_addr);
		setMulticast.imr_interface.S_un.S_addr = htonl(INADDR_ANY);
		check = setsockopt(m_udpReciveSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&setMulticast, sizeof(setMulticast));
		if (check == SOCKET_ERROR)
		{
			std::cout << "Server: Failed to set assign multicast on udp on server, ErrorCode: " << WSAGetLastError() << std::endl;
		}

		check = setsockopt(m_udpReciveSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&ttl, sizeof(ttl));
		if (check == SOCKET_ERROR)
		{
			std::cout << "Server: Failed to set ttl on udp on server, ErrorCode: " << WSAGetLastError() << std::endl;
		}
	}

	struct Client::UdpReturnData Client::SendandReciveUdp(PlayerNetworkComponent input)
	{
		input.udpId = m_sendUdpId++;
		memcpy(m_sendUdpBuffer, &input, sizeof(input));
		sendto(m_udpSendSocket, (char*)m_sendUdpBuffer, sizeof(input), 0, (struct sockaddr*)&m_hostAddressUdp, sizeof(m_hostAddressUdp));

		int bytesRecived = 0, hostAddressLength = sizeof(m_reciveAddressUdp);
		UdpData holdAll;
		UdpReturnData returnData;

		bytesRecived = recvfrom(m_udpReciveSocket, (char*)m_reciveUdpBuffer, 1024, 0, (struct sockaddr*)&m_reciveAddressUdp, &hostAddressLength);
		if (bytesRecived > 0)
		{
			memcpy(&holdAll, m_reciveUdpBuffer, sizeof(holdAll));
			if (holdAll.udpId > m_udpId)
			{
				m_udpId = holdAll.udpId;
				memcpy(&returnData.m_holdplayersUdp, m_reciveUdpBuffer + sizeof(holdAll), sizeof(returnData.m_holdplayersUdp));
			}

		}
		return returnData;
	}

	void Client::SendUdp(PlayerNetworkComponent input)
	{
		int bytesSend;
		input.udpId = m_sendUdpId++;
		memcpy(m_sendUdpBuffer, &input, sizeof(input));
		bytesSend = sendto(m_udpSendSocket, (char*)m_sendUdpBuffer, sizeof(input), 0, (struct sockaddr*)&m_hostAddressUdp, sizeof(m_hostAddressUdp));
	}

	struct Client::UdpReturnData Client::ReciveUdp()
	{
		int bytesRecived = 0, hostAddressLength = sizeof(m_reciveAddressUdp);
		UdpData holdAll;
		UdpReturnData returnData;
		
		bytesRecived = recvfrom(m_udpReciveSocket, (char*)m_reciveUdpBuffer, 1024, 0, (struct sockaddr*)&m_reciveAddressUdp, &hostAddressLength);
		if (bytesRecived > 0)
		{
			memcpy(&holdAll, m_reciveUdpBuffer, sizeof(holdAll));
			if (holdAll.udpId > m_udpId)
			{
				m_udpId = holdAll.udpId;
				memcpy(&returnData.m_holdplayersUdp, m_reciveUdpBuffer + sizeof(holdAll), sizeof(returnData.m_holdplayersUdp));
			}
			
		}
		return returnData;
	}