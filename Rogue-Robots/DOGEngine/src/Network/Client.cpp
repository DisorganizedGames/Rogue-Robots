#include "Client.h"
namespace DOG
{
	Client::Client()
	{
		m_connectSocket = INVALID_SOCKET;
	}

	Client::~Client()
	{

	}

	int Client::ConnectTcpServer(std::string ipAdress)
	{

		WSADATA socketStart;
		SOCKET connectSocket = INVALID_SOCKET;
		addrinfo client, * addrOutput = NULL, * ptr = NULL;
		BOOL turn = TRUE;
		char* ipAdressInput = new char[sizeof(ipAdress) + 1];
		char* inputSend = new char[sizeof(int)];
		int check;

		strcpy(ipAdressInput, ipAdress.c_str());

		ZeroMemory(&client, sizeof(client));
		client.ai_family = AF_INET;
		client.ai_socktype = SOCK_STREAM;
		client.ai_protocol = IPPROTO_TCP;


		check = WSAStartup(0x202, &socketStart);
		assert(check == 0);

		check = getaddrinfo(ipAdressInput, "50005", &client, &addrOutput);
		assert(check == 0);

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

		char optval[1];
		socklen_t optlen = sizeof(optval);

		//setsockopt(m_connectSocket, SOL_SOCKET, SO_KEEPALIVE, (char*)&turn, sizeof(bool));
		setsockopt(m_connectSocket, SOL_SOCKET, TCP_NODELAY, (char*)&turn, sizeof(bool));

		//get player number
		check = recv(m_connectSocket, inputSend, sizeof(int), 0);

		if (atoi(inputSend) == -1)
		{
			std::cout << "\nCLient: Server Full: " << atoi(inputSend) << std::endl;
			return -1;
		}

		std::cout << "\nCLient: Player nr: " << atoi(inputSend) << std::endl;
		assert(m_connectSocket != INVALID_SOCKET);

		std::cout << "CLient: Waiting for more players to connect..." << std::endl;
		check = recv(m_connectSocket, inputSend, sizeof(int), 0);

		while (atoi(inputSend) != -2)
		{
			check = recv(m_connectSocket, inputSend, sizeof(int), 0);
			continue;
		}
		std::cout << "CLient: All players connected, Starting " << std::endl;


		return atoi(inputSend);
	}

	struct Client::ClientsData* Client::SendandReciveTcp(ClientsData input) {
		char* inputSend = new char[sizeof(ClientsData)];
		char recvbuf[1024];
		memcpy(inputSend, &input, sizeof(ClientsData));
		send(m_connectSocket, inputSend, sizeof(ClientsData), 0);
		recv(m_connectSocket, recvbuf, sizeof(recvbuf), 0);
		memcpy(m_playersClient, recvbuf, sizeof(m_playersClient));
		return m_playersClient;
	}



	struct Client::ClientsData* Client::GetClientsData()
	{
		return m_playersClient;
	}

}