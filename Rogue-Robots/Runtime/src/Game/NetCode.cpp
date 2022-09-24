#include "NetCode.h"
using namespace DOG;
NetCode::NetCode()
{
	m_netCodeAlive = TRUE;
	m_Output = nullptr;
	m_input.rotation = DirectX::XMVectorSet(0, 0, 0, 0);
	m_input.position = DirectX::XMVectorSet(0, 0, 0, 0);
	m_active = FALSE;
	m_startUp = FALSE;
	m_thread = std::thread(&NetCode::Recive, this);

}

NetCode::~NetCode()
{
	m_netCodeAlive = FALSE;
	m_thread.join();
}


void NetCode::OnUpdate(std::shared_ptr<MainPlayer> player)
{

	if (m_active)
	{
		//if connects first time 
		if (m_startUp == TRUE)
		{

			EntityManager::Get().Collect<TransformComponent, NetworkPlayerComponent>().Do([&](TransformComponent& transformC, NetworkPlayerComponent& networkC)
				{
					//Give player correct model
					if (networkC.playerId == m_input.playerId)
						player->SetPosition(m_Output[networkC.playerId].position);
				});

			m_startUp = false;
		}

	//if connected to server
		AddPosition(player->GetPosition());
		AddRotation(player->GetRotation());
		EntityManager::Get().Collect<TransformComponent, NetworkPlayerComponent>().Do([&](TransformComponent& transformC, NetworkPlayerComponent& networkC)
			{
				if (networkC.playerId == m_input.playerId)
				{
					transformC.SetPosition(player->GetPosition());
					transformC.SetRotation(player->GetRotation());
				}
				else
				{
					transformC.SetPosition(m_Output[networkC.playerId].position);
					transformC.SetRotation(m_Output[networkC.playerId].rotation);
				}
			});
	}
}

void NetCode::Recive()
{
	DOG::Server serverTest;
	bool start = FALSE;
	char input = 'o';
	while (start == FALSE)
	{
		
		std::cout << "\nInput 'h' to host, 'j' to join, 'o' to play offline: ";
		//input = getchar(); // uncomment to startup online
		switch (input)
		{
		case 'h':
		{
			bool server	= serverTest.StartTcpServer();
			if (server)
			{
				// join server
				std::string ip = serverTest.GetIpAddress();
				if (ip != "")
				{
					std::cout << "Hosting at: " << ip << std::endl;
					m_input.playerId = m_client.ConnectTcpServer(ip);
					if (m_input.playerId > -1)
					{
						m_input.position = DirectX::XMVectorSet(0, m_input.playerId * 2 + 1, 0, 0);
						m_Output = m_client.ReciveTcp();
						start = TRUE;
					}
				}
			}
			break;
		}
		case 'j':
		{
			std::cout << "Enter ip address to host('d' to connect to default ip address): ";
			std::string inputString;
			fseek(stdin, 0, SEEK_END);
			std::cin >> inputString;
			if (inputString[0] == 'd')
			{
				m_input.playerId = m_client.ConnectTcpServer("192.168.50.214"); //192.168.1.55 || 192.168.50.214
			}
			else
			{
				m_input.playerId = m_client.ConnectTcpServer(inputString);
			}
			if (m_input.playerId > -1)
			{
				m_input.position = DirectX::XMVectorSet(0, m_input.playerId * 2 + 1, 0, 0);
				m_Output = m_client.ReciveTcp();
				start = TRUE;
			}
			break;
		}
		case 'o':
		{
			std::cout << " Offline mode Selected" << std::endl;
			start = TRUE;
			while (m_netCodeAlive)
				continue;
			break;
		}
		default:
			break;
		}
		if(start == FALSE)
			std::cout << "Failed input, try agiain" << std::endl;
		fseek(stdin, 0, SEEK_END);
	}
	while (m_netCodeAlive)
	{
		m_mut.lock();
		DOG::Client::ClientsData testInput = m_input;
		m_input = m_client.CleanClientsData(m_input);
		m_mut.unlock();
		m_client.SendTcp(testInput);
		m_Output = m_client.ReciveTcp();
		if (m_active == FALSE)
		{
			m_startUp = TRUE;
			m_active = TRUE;
		}

	}
}


void NetCode::AddPlayersId(std::vector<DOG::entity> playersId)
{
	m_playersId = playersId;
}

void NetCode::AddMatrix(DirectX::XMMATRIX input)
{
	m_mut.lock();
	m_input.matrix = input;
	m_mut.unlock();
}

void NetCode::AddRotation(DirectX::SimpleMath::Vector3 input)
{
	m_mut.lock();
	m_input.rotation = input;
	m_mut.unlock();

}

void NetCode::AddPosition(DirectX::SimpleMath::Vector3 input)
{
	m_mut.lock();
	input = DirectX::XMVectorSubtract(input, m_input.position);
	m_input.position = DirectX::XMVectorAdd(m_input.position, input);
	m_mut.unlock();
}
