#include "NetCode.h"
using namespace DOG;
NetCode::NetCode()
{


	m_netCodeAlive = TRUE;
	m_outputTcp = nullptr;
	m_inputTcp.matrix = {};
	m_inputTcp.nrOfNetTransform = 0;
	m_inputTcp.nrOfNetStats = 0;
	m_playerInputUdp.playerId = 0;
	m_playerInputUdp.matrix = {};
	m_playerInputUdp.shoot = FALSE;
	m_playerInputUdp.jump = FALSE;
	m_playerInputUdp.activateActiveItem = FALSE;

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


void NetCode::OnUpdate()
{
	EntityManager::Get().Collect<ThisPlayer, TransformComponent>().Do([&](ThisPlayer&, TransformComponent& transC)
	{
				AddMatrixTcp(transC.worldMatrix);
				AddMatrixUdp(transC.worldMatrix);
	});

	if (m_active)
	{
		if (m_startUp == TRUE)
		{

			
			DOG::EntityManager& m_entityManager = DOG::EntityManager::Get();
			EntityManager::Get().Collect<NetworkPlayerComponent, ThisPlayer, TransformComponent>().Do([&](entity id, NetworkPlayerComponent& networkC, ThisPlayer&, TransformComponent& transC)
				{
					if (networkC.playerId != m_inputTcp.playerId)
					{
						m_entityManager.AddComponent<OnlinePlayer>(id);
						m_entityManager.RemoveComponent<ThisPlayer>(id);
						m_entityManager.RemoveComponent<CameraComponent>(id);
						m_entityManager.RemoveComponent<AudioListenerComponent>(id);
						transC.worldMatrix = m_outputTcp[networkC.playerId].matrix;
					}

				});
			EntityManager::Get().Collect<NetworkPlayerComponent, TransformComponent, OnlinePlayer>().Do([&](entity id, NetworkPlayerComponent& networkC, TransformComponent& transC, OnlinePlayer&)
				{
					if (networkC.playerId == m_inputTcp.playerId)
					{
						m_entityManager.AddComponent<ThisPlayer>(id);
						m_entityManager.AddComponent<CameraComponent>(id);
						m_entityManager.AddComponent<AudioListenerComponent>(id);
						m_entityManager.RemoveComponent<OnlinePlayer>(id);
					}
					transC.worldMatrix = m_outputTcp[networkC.playerId].matrix;
				});
			
			m_startUp = false;
		}



		EntityManager::Get().Collect<NetworkPlayerComponent, ThisPlayer, InputController>().Do([&](NetworkPlayerComponent&, ThisPlayer&, InputController& inputC)
			{

				m_playerInputUdp.shoot = inputC.shoot;
				m_playerInputUdp.jump = inputC.jump;
				m_playerInputUdp.activateActiveItem = inputC.activateActiveItem;
				m_playerInputUdp.switchComp = inputC.switchComp;

			});

		EntityManager::Get().Collect<TransformComponent, NetworkPlayerComponent, InputController, OnlinePlayer>().Do([&](TransformComponent& transformC, NetworkPlayerComponent& networkC, InputController& inputC, OnlinePlayer&)
			{
				transformC.worldMatrix = m_outputUdp.m_holdplayersUdp[networkC.playerId].matrix;
				transformC.SetScale(DirectX::SimpleMath::Vector3(0.5f, 0.5f, 0.5f));
				inputC.shoot = m_outputUdp.m_holdplayersUdp[networkC.playerId].shoot;
				inputC.jump = m_outputUdp.m_holdplayersUdp[networkC.playerId].jump;
				inputC.activateActiveItem = m_outputUdp.m_holdplayersUdp[networkC.playerId].activateActiveItem;
				inputC.switchComp = m_outputUdp.m_holdplayersUdp[networkC.playerId].switchComp;
				

			});


	}
}

void NetCode::Recive()
{
	Server serverHost;
	
	bool start = FALSE;
	char input = 'o';
	while (start == FALSE)
	{

		std::cout << "\nInput 'h' to host, 'j' to join, 'o' to play offline: ";
		input = getchar(); // uncomment to startup online
		switch (input)
		{
		case 'h':
		{
			bool server = serverHost.StartTcpServer();
			if (server)
			{
				// join server
				std::string ip = serverHost.GetIpAddress();
				if (ip != "")
				{
					std::cout << "Hosting at: " << ip << std::endl;
					m_inputTcp.playerId = m_client.ConnectTcpServer(ip);
					if (m_inputTcp.playerId > -1)
					{
						m_client.SendTcp(m_inputTcp);
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
			//fseek(stdin, 0, SEEK_END);
			std::cin >> inputString;
			if (inputString[0] == 'd')
			{
				m_inputTcp.playerId = m_client.ConnectTcpServer("192.168.1.74"); //192.168.1.55 || 192.168.50.214
			}
			else
			{
				m_inputTcp.playerId = m_client.ConnectTcpServer(inputString);
			}
			if (m_inputTcp.playerId > -1)
			{
				
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
		if (start == FALSE)
		{
			std::cout << "Failed input, try agiain" << std::endl;
			//fseek(stdin, 0, SEEK_END);
		}
	}
	m_threadUdp = std::thread(&NetCode::ReciveUdp, this);
	if (m_netCodeAlive)
	{
		if (m_active == false)
		{
			m_startUp = true;
			m_active = true;
			
		}
		//tcp
		
		while (m_netCodeAlive)
		{
			char sendBuffer[SEND_AND_RECIVE_BUFFER_SIZE];
			char* reciveBuffer = new char[SEND_AND_RECIVE_BUFFER_SIZE];
			m_inputTcp.nrOfNetTransform = 0;
			m_inputTcp.nrOfNetStats = 0;
			int bufferSize = 0;
			//If player got valid id from server else idle
			if (m_inputTcp.playerId > -1)
			{
				bufferSize += sizeof(Client::ClientsData);
				//sync all transforms Host only
				if (m_inputTcp.playerId == 0)
				{
					EntityManager::Get().Collect<NetworkTransform, TransformComponent>().Do([&](entity id, NetworkTransform& netC, TransformComponent& transC)
						{
							netC.objectId = id;
							netC.transform = transC.worldMatrix;
							memcpy(sendBuffer + bufferSize, &netC, sizeof(NetworkTransform));
							m_inputTcp.nrOfNetTransform++;
							bufferSize += sizeof(NetworkTransform);

						});
				}
				//Sync all stats component that this player has hit
				EntityManager::Get().Collect<NetworkAgentStats, AgentStatsComponent>().Do([&](NetworkAgentStats& netC, AgentStatsComponent& aStats)
					{
						if (netC.playerId = netC.playerId)
						{
							netC.stats = aStats;
							memcpy(sendBuffer + bufferSize, &netC, sizeof(NetworkTransform));
							m_inputTcp.nrOfNetStats++;
							bufferSize += sizeof(NetworkTransform);
						}
					});
				//put in the client data
				memcpy(sendBuffer, (char*)&m_inputTcp, sizeof(m_inputTcp));
				
				m_client.SendChararrayTcp(sendBuffer, bufferSize);
				reciveBuffer = m_client.ReciveCharArrayTcp(reciveBuffer);
				if (reciveBuffer == nullptr)
				{
					std::cout << "bad tcp packet \n"; 
				}
				else
				{
					int bufferReciveSize = 0;
					//�pdate the players entites stats
					memcpy(m_outputTcp, reciveBuffer, sizeof(Client::ClientsData) * MAX_PLAYER_COUNT);

					//Update the transfroms, Only none hosts
					
					if (m_inputTcp.playerId > 0 && m_outputTcp->playerId < MAX_PLAYER_COUNT)
					{
						NetworkTransform* temp = new NetworkTransform;
						memcpy(temp, reciveBuffer + sizeof(Client::ClientsData) * MAX_PLAYER_COUNT, sizeof(NetworkTransform));
						EntityManager::Get().Collect<NetworkTransform, TransformComponent>().Do([&](entity id, NetworkTransform& networkC, TransformComponent& transC)
							{
								for (int i = 0; i < m_outputTcp[0].nrOfNetTransform; ++i)
								{
									for (int i = 0; i < m_outputTcp[0].nrOfNetTransform; ++i)
									{
										memcpy(temp, reciveBuffer + bufferReciveSize + sizeof(NetworkTransform) * i, sizeof(NetworkTransform));
										if (id == temp->objectId)
										{
											transC.worldMatrix = temp->transform;
										}

									}
								});
						}
					}
				}
				
			}
			delete[] reciveBuffer;
		}
	}
	
}

void NetCode::ReciveUdp()
{
	m_playerInputUdp.playerId = m_inputTcp.playerId;
	while (m_netCodeAlive)
	{
		m_mut.lock();
		m_client.SendUdp(m_playerInputUdp);
		m_mut.unlock();
		m_outputUdp = m_client.ReciveUdp();
	}

}

void NetCode::AddMatrixTcp(DirectX::XMMATRIX input)
{
	m_mut.lock();
	m_inputTcp.matrix = input;
	m_mut.unlock();
}


void NetCode::AddMatrixUdp(DirectX::XMMATRIX input) 
{
	m_mut.lock();
	m_playerInputUdp.matrix = input; 
	m_mut.unlock();
}


