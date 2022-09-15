#include "Client.h"
namespace DOG
{
	Client::Client()
	{
		ClientsData test;
		m_connectSocket = INVALID_SOCKET;
	}

	Client::~Client()
	{
	
	}

	int Client::ConnectTcpServer(std::string ipAdress)
	{
		WSADATA socketStart;
		addrinfo client, * addrOutput = NULL, * ptr = NULL;
		BOOL turn = TRUE;
		char* ipAdressInput = new char[sizeof(ipAdress) + 1];
		char* inputSend = new char[sizeof(int)];
		int check, returnValue;

		std::ranges::copy(ipAdress, ipAdressInput);

		ZeroMemory(&client, sizeof(client));
		client.ai_family = AF_INET;
		client.ai_socktype = SOCK_STREAM;
		client.ai_protocol = IPPROTO_TCP;


		check = WSAStartup(0x202, &socketStart);
		if (check != 0)
		{
			throw std::runtime_error("Failed to start WSA on client");
		}

		check = getaddrinfo(ipAdressInput, "50005", &client, &addrOutput);
		if (check != 0)
		{
			throw std::runtime_error("Failed to get address info on client");
		}

		//connect to server
		for (ptr = addrOutput; ptr != NULL; ptr = ptr->ai_next)
		{
			m_connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
			assert(m_connectSocket != INVALID_SOCKET);

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
		assert(m_connectSocket != INVALID_SOCKET);

		//set socket to tcp_nodelay
		setsockopt(m_connectSocket, SOL_SOCKET, TCP_NODELAY, (char*)&turn, sizeof(bool));

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
			assert(m_connectSocket != INVALID_SOCKET);
			return returnValue +1;
		}
	}


	struct Client::ClientsData* Client::SendandReciveTcp(ClientsData input) {
		char* inputSend = new char[sizeof(ClientsData)];
		char recvbuf[2048];
		memcpy(inputSend, &input, sizeof(ClientsData));
		send(m_connectSocket, inputSend, sizeof(ClientsData), 0);
		recv(m_connectSocket, recvbuf, sizeof(recvbuf), 0);
		memcpy(m_playersClient, recvbuf, sizeof(m_playersClient));
		delete[] inputSend;
		return m_playersClient;
	}


	struct Client::ClientsData Client::AddString(ClientsData player, std::string inputs)
	{
		for (int i = 0; i < inputs.length(); i++)
		{
			if (i > 63)
				break;
			player.inputs[player.inputLength] = inputs.at(i);
			player.inputLength++;
		}
		
		return player;
	}

	struct Client::ClientsData Client::CleanClientsData(ClientsData player)
	{
		memset(player.inputs, 0, sizeof(player.inputs));
		player.inputLength = 0;
		return player;
	}
}