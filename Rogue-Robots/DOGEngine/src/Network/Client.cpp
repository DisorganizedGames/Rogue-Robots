#include "Client.h"
namespace DOG
{
	Client::Client()
	{
		ClientsData test;
		m_connectSocket = INVALID_SOCKET;
		m_inputSend = new char[sizeof(ClientsData)];
	}

	Client::~Client()
	{
		delete[] m_inputSend;
	}

	int Client::ConnectTcpServer(std::string ipAdress)
	{
		WSADATA socketStart;
		addrinfo client, * addrOutput = NULL, * ptr = NULL;
		BOOL turn = TRUE;
		char* ipAdressInput = new char[sizeof(ipAdress) + 1];
		char* inputSend = new char[sizeof(int)];
		int check, returnValue;

		//std::ranges::copy(ipAdress, ipAdressInput);
		strcpy_s(ipAdressInput, sizeof(ipAdress), ipAdress.c_str());

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

		check = getaddrinfo(ipAdressInput, "50005", &client, &addrOutput);
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
		delete[] ipAdressInput;
		delete[] inputSend;

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
		recv(m_connectSocket, recvbuf, sizeof(recvbuf), 0);
		memcpy(m_playersClient, recvbuf, sizeof(m_playersClient));
		return m_playersClient;
	}

	struct Client::ClientsData* Client::SendandReciveTcp(ClientsData input) 
	{
		char recvbuf[2048];
		memcpy(m_inputSend, &input, sizeof(ClientsData));
		send(m_connectSocket, m_inputSend, sizeof(ClientsData), 0);
		recv(m_connectSocket, recvbuf, sizeof(recvbuf), 0);
		memcpy(m_playersClient, recvbuf, sizeof(m_playersClient));
		return m_playersClient;
	}

	struct Client::ClientsData Client::CleanClientsData(ClientsData player)
	{
		memset(player.inputs, 0, sizeof(player.inputs));
		player.inputLength = 0;
		//player.rotation = DirectX::XMVectorSet(0, 0, 0, 0);
		return player;
	}
}