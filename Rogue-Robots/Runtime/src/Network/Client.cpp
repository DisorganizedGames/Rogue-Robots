#include "Client.h"

Client::Client()
{
	ClientsData test;
	m_connectSocket = INVALID_SOCKET;
	m_inputSend = new char[sizeof(ClientsData)];
	m_hostIp = new char[64];
	m_sendUdpBuffer = new char[sizeof(PlayerNetworkComponentUdp)];
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

INT8 Client::ConnectTcpServer(std::string ipAdress)
{
	WSADATA socketStart;
	addrinfo client, * addrOutput = NULL, * ptr = NULL;
	BOOL turn = TRUE;
	char inputSend[sizeof(int)];
	int check;
	INT8 returnValue;

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

	check = getaddrinfo(m_hostIp, PORTNUMBER_OUT, &client, &addrOutput);
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

	//set socket to tcp_nodelay
	DWORD ttl = 5020;
	check = setsockopt(m_connectSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&ttl, sizeof(DWORD));
	if (check == SOCKET_ERROR)
	{
		std::cout << "Client: Failed to set time to live to tcp_nodelay on client, ErrorCode: " << WSAGetLastError() << std::endl;
		return -1;
	}

	//get player number
	check = recv(m_connectSocket, inputSend, sizeof(int), 0);
	returnValue = (INT8)atoi(inputSend);

	SetUpUdp();
	if (returnValue == -1)
	{
		std::cout << "\nCLient: Server Full: " << returnValue << std::endl;
		return -1;
	}
	else
	{
		std::cout << "\nCLient: Player nr: " << returnValue + 1 << std::endl;
		return returnValue;
	}
}


void Client::SendChararrayTcp(char* input, int size)
{
	send(m_connectSocket, input, size, 0);
	return;
}


u8 Client::ReceiveCharArrayTcp(char* reciveBuffer)
{
	TcpHeader packet;
	int bytesRecived, processedBytes = 0;
	bool isItFirstTime = true;
	while (true)
	{
		bytesRecived = recv(m_connectSocket, reciveBuffer + processedBytes, SEND_AND_RECIVE_BUFFER_SIZE - processedBytes, 0);

		if (bytesRecived > 0)
		{
			//read in header
			if (isItFirstTime)
			{
				isItFirstTime = false;
				memcpy(&packet, reciveBuffer, sizeof(TcpHeader));
			}
			//if correct return the packet
			if (bytesRecived + processedBytes == packet.sizeOfPayload)
			{
				return 1;
			}
			//multiple packets detected
			else if ((bytesRecived - processedBytes) > packet.sizeOfPayload)
			{
				u8 nrOfPackets = 0;

				while ( (bytesRecived - processedBytes) > 0)
				{
					processedBytes += packet.sizeOfPayload;
					memcpy(&packet, reciveBuffer + processedBytes, sizeof(TcpHeader));
					nrOfPackets++;
				}
				return nrOfPackets;
			}
			// only part of  the packet arrived
			else if ((bytesRecived - processedBytes) < packet.sizeOfPayload)
			{
				std::cout << "Client: Only part of the packet arrived: " << bytesRecived << "header payload: " << packet.sizeOfPayload << std::endl;
				processedBytes += bytesRecived;
			}
		}
		else if (bytesRecived == -1)
		{
			std::cout << "Client: Error reciving tcp packet: " << WSAGetLastError() << std::endl;
			return 0;
		}
	}
}


void Client::SetUpUdp()
{
	int check = 0;
	DWORD ttl = 5020;
	m_udpSendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (m_udpSendSocket == INVALID_SOCKET)
	{
		std::cout << "Client: Failed to create udpSocket on client, ErrorCode: " << WSAGetLastError() << std::endl;
		return;
	}


	m_hostAddressUdp.sin_family = AF_INET;
	inet_pton(AF_INET, MULTICAST_ADRESS, &m_hostAddressUdp.sin_addr.s_addr); //inet_addr("239.255.255.0");
	m_hostAddressUdp.sin_port = htons(PORTNUMBER_IN_INT);

	//recive
	bool turn = TRUE;
	struct ip_mreq setMulticast;
	m_udpReciveSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (m_udpReciveSocket == INVALID_SOCKET)
	{
		std::cout << "Client: Failed to create udpSocket on client, ErrorCode: " << WSAGetLastError() << std::endl;
		return;
	}

	check = setsockopt(m_udpReciveSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&turn, sizeof(bool));
	if (check == SOCKET_ERROR)
	{
		std::cout << "Client: Failed to set udpsocket to reusabale adress to unblocking on server, ErrorCode: " << WSAGetLastError() << std::endl;
		return;
	}


	m_reciveAddressUdp.sin_family = AF_INET;
	m_reciveAddressUdp.sin_addr.s_addr = htonl(INADDR_ANY);
	m_reciveAddressUdp.sin_port = htons(PORTNUMBER_OUT_INT);

	check = bind(m_udpReciveSocket, (struct sockaddr*)&m_reciveAddressUdp, sizeof(m_reciveAddressUdp));
	if (check == SOCKET_ERROR)
	{
		std::cout << "Server: Failed to bind udpsocket on server, ErrorCode: " << WSAGetLastError() << std::endl;
		return;
	}

	inet_pton(AF_INET, MULTICAST_ADRESS, &setMulticast.imr_multiaddr.S_un.S_addr);
	setMulticast.imr_interface.S_un.S_addr = htonl(INADDR_ANY);
	check = setsockopt(m_udpReciveSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&setMulticast, sizeof(setMulticast));
	if (check == SOCKET_ERROR)
	{
		std::cout << "Server: Failed to set assign multicast on udp on server, ErrorCode: " << WSAGetLastError() << std::endl;
		return;
	}

	check = setsockopt(m_udpReciveSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&ttl, sizeof(ttl));
	if (check == SOCKET_ERROR)
	{
		std::cout << "Server: Failed to set ttl on udp on server, ErrorCode: " << WSAGetLastError() << std::endl;
		return;
	}
}

struct UdpReturnData Client::SendandReciveUdp(PlayerNetworkComponentUdp input)
{
	input.udpId = m_sendUdpId++;
	sendto(m_udpSendSocket, (char*)&input, sizeof(input), 0, (struct sockaddr*)&m_hostAddressUdp, sizeof(m_hostAddressUdp));

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

void Client::SendUdp(PlayerNetworkComponentUdp input)
{
	input.udpId = ++m_sendUdpId;
	sendto(m_udpSendSocket, (char*)&input, sizeof(input), 0, (struct sockaddr*)&m_hostAddressUdp, sizeof(m_hostAddressUdp));
}

struct UdpReturnData Client::ReceiveUdp()
{
	int bytesRecived = 0, hostAddressLength = sizeof(m_reciveAddressUdp);
	UdpData header;
	UdpReturnData returnData;

	bytesRecived = recvfrom(m_udpReciveSocket, (char*)m_reciveUdpBuffer, SEND_AND_RECIVE_BUFFER_SIZE, 0, (struct sockaddr*)&m_reciveAddressUdp, &hostAddressLength);
	if (bytesRecived > 0)
	{
		memcpy(&header, m_reciveUdpBuffer, sizeof(header));
		if (header.udpId > m_udpId)
		{
			m_udpId = header.udpId;
			memcpy(&returnData.m_holdplayersUdp, m_reciveUdpBuffer + sizeof(header), sizeof(returnData.m_holdplayersUdp));
		}

	}
	return returnData;
}