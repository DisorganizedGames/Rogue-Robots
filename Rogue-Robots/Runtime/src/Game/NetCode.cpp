#include "NetCode.h"
using namespace DOG;
NetCode::NetCode()
{
	

	m_netCodeAlive = TRUE;
	m_outputTcp = nullptr;
	m_inputTcp.rotation = DirectX::XMVectorSet(0, 0, 0, 0);
	m_inputTcp.position = DirectX::XMVectorSet(0, 0, 0, 0);
	

	m_playerInputUdp.playerId = 0;
	m_playerInputUdp.playerOptions = 0;
	m_playerInputUdp.rotation = DirectX::XMVectorSet(0, 0, 0, 0);
	m_playerInputUdp.position = DirectX::XMVectorSet(0, 0, 0, 0);
	m_hardSyncTcp = FALSE;
	m_active = FALSE;
	m_startUp = FALSE;
	m_thread = std::thread(&NetCode::Recive, this);
	

}

NetCode::~NetCode()
{
	m_netCodeAlive = FALSE;
	m_thread.join();
	m_threadUdp.join();
}


void NetCode::OnUpdate(std::shared_ptr<MainPlayer> player)
{

	if (m_active)
	{
	
		if (m_startUp == TRUE)
		{
			m_playerInputUdp.playerId = m_inputTcp.playerId;
			EntityManager::Get().Collect<NetworkPlayerComponent>().Do([&](NetworkPlayerComponent& networkC)
				{
					//Give player correct model
					if (networkC.playerId == m_inputTcp.playerId)
						player->SetPosition(m_outputTcp[networkC.playerId].position);
				});
			EntityManager::Get().Collect<TransformComponent, NetworkPlayerComponent>().Do([&](TransformComponent& transformC, NetworkPlayerComponent& networkC)
				{
					if (networkC.playerId != m_inputTcp.playerId)
					{
						transformC.SetPosition(m_outputTcp[networkC.playerId].position);
						transformC.SetRotation(m_outputTcp[networkC.playerId].rotation);
					}
				});

			m_startUp = false;
		}

		AddPositionTcp(player->GetPosition());
		AddRotationTcp(player->GetRotation());

		AddPositionUdp(player->GetPosition());
		AddRotationUdp(player->GetRotation());
		//TODO: when player entity exist
		//EntityManager::Get().Collect<mainPlayer>().Do([&](mainPlayer& playerC)
		//{
		//	AddMatrixUdp(playerC.worldMatrix);
		//};
	
			EntityManager::Get().Collect<TransformComponent, NetworkPlayerComponent>().Do([&](TransformComponent& transformC, NetworkPlayerComponent& networkC)
				{
					//Udp
					if (networkC.playerId == m_inputTcp.playerId)
					{
						transformC.SetPosition(player->GetPosition());
						transformC.SetRotation(player->GetRotation());
					}
					else
					{
						for (int i = 0; i < MAX_PLAYER_COUNT; i++)
						{
							if (networkC.playerId == m_outputUdp.m_holdplayersUdp[i].playerId)
							{
								transformC.SetPosition(m_outputUdp.m_holdplayersUdp[i].position);
								transformC.SetRotation(m_outputTcp[networkC.playerId].rotation);
							}
						}
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
					m_inputTcp.playerId = m_client.ConnectTcpServer(ip);
					if (m_inputTcp.playerId > -1)
					{
						m_inputTcp.position = DirectX::XMVectorSet(0, (float)(m_inputTcp.playerId * 2 + 1), 0, 0);
						m_outputTcp = m_client.ReciveTcp();
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
				m_inputTcp.playerId = m_client.ConnectTcpServer("192.168.1.55"); //192.168.1.55 || 192.168.50.214
			}
			else
			{
				m_inputTcp.playerId = m_client.ConnectTcpServer(inputString);
			}
			if (m_inputTcp.playerId > -1)
			{
				m_inputTcp.position = DirectX::XMVectorSet(0, (float)(m_inputTcp.playerId * 2 + 1), 0, 0);
				m_outputTcp = m_client.ReciveTcp();
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
	m_threadUdp = std::thread(&NetCode::ReciveUdp, this);
	if (m_netCodeAlive)
	{
		if (m_active == FALSE)
		{
			m_startUp = TRUE;
			m_active = TRUE;
		}
		//tcp
		while (m_netCodeAlive)
		{
			m_mut.lock();
			DOG::Client::ClientsData testInput = m_inputTcp;
			m_mut.unlock();
			m_client.SendTcp(testInput);
			m_outputTcp = m_client.ReciveTcp();
		}
	}
}

void NetCode::ReciveUdp()
{
	while (m_netCodeAlive)
	{
		m_mut.lock();
		DOG::Client::PlayerNetworkComponent holdUdp = m_playerInputUdp;
		m_mut.unlock();
		m_outputUdp = m_client.SendandReciveUdp(holdUdp);
	}

}

void NetCode::AddPlayersId(std::vector<DOG::entity> playersId)
{
	m_playersId = playersId;
}

//void NetCode::AddMatrixTcp(DirectX::XMMATRIX input)
//{
//	m_mut.lock();
//	m_inputTcp.matrix = input;
//	m_mut.unlock();
//}



void NetCode::AddRotationTcp(DirectX::SimpleMath::Vector3 input)
{
	m_mut.lock();
	m_inputTcp.rotation = input;
	m_mut.unlock();

}

void NetCode::AddPositionTcp(DirectX::SimpleMath::Vector3 input)
{
	m_mut.lock();
	m_inputTcp.position = input;
	m_mut.unlock();
}

void NetCode::AddRotationUdp(DirectX::SimpleMath::Vector3 input)
{
	m_mut.lock();
	m_playerInputUdp.rotation = input;
	m_mut.unlock();

}

void NetCode::AddPositionUdp(DirectX::SimpleMath::Vector3 input)
{
	m_mut.lock();
	input = DirectX::XMVectorSubtract(input, m_playerInputUdp.position);
	m_playerInputUdp.position = DirectX::XMVectorAdd(m_playerInputUdp.position, input);
	//m_playerInputUdp.position = input;
	m_mut.unlock();
}

//void NetCode::AddMatrixUdp(DirectX::XMMATRIX input) change back later when main player has transform
//{
//	m_mut.lock();
//	m_inputUdp.playerMatrix.push_back(input); 
//	m_mut.unlock();
//}
