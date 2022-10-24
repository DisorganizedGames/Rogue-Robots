#include "NetCode.h"
using namespace DOG;
NetCode::NetCode()
{


	m_netCodeAlive = TRUE;
	m_outputTcp = nullptr;
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
	

	m_bufferSize = 0;
	m_bufferReceiveSize = 0;
	m_receiveBuffer = new char[SEND_AND_RECIVE_BUFFER_SIZE];
	m_dataIsReadyToBeSentTcp = false;
	m_dataIsReadyToBeReceivedTcp = false;
}

NetCode::~NetCode()
{
	m_netCodeAlive = FALSE;
	if(m_thread.joinable())
		m_thread.join();
	if (m_threadUdp.joinable())
		m_threadUdp.join();
	
	delete[] m_receiveBuffer;
}


void NetCode::OnUpdate()
{
	EntityManager::Get().Collect<ThisPlayer, TransformComponent>().Do([&](ThisPlayer&, TransformComponent& transC)
		{
			AddMatrixUdp(transC.worldMatrix);
		});

	if (m_active)
	{
		DOG::EntityManager& m_entityManager = DOG::EntityManager::Get();
		if (m_startUp == true)
		{


			EntityManager::Get().Collect<NetworkPlayerComponent, ThisPlayer, TransformComponent>().Do([&](entity id, NetworkPlayerComponent& networkC, ThisPlayer&, TransformComponent&)
				{
					if (networkC.playerId != m_inputTcp.playerId)
					{
						m_entityManager.AddComponent<OnlinePlayer>(id);
						m_entityManager.RemoveComponent<ThisPlayer>(id);
						m_entityManager.RemoveComponent<CameraComponent>(id);
						m_entityManager.RemoveComponent<AudioListenerComponent>(id);
					}

				});
			EntityManager::Get().Collect<NetworkPlayerComponent, TransformComponent, OnlinePlayer>().Do([&](entity id, NetworkPlayerComponent& networkC, TransformComponent&, OnlinePlayer&)
				{
					if (networkC.playerId == m_inputTcp.playerId)
					{
						m_entityManager.AddComponent<ThisPlayer>(id);
						m_entityManager.AddComponent<CameraComponent>(id);
						m_entityManager.AddComponent<AudioListenerComponent>(id);
						m_entityManager.RemoveComponent<OnlinePlayer>(id);
					}
				});

			m_startUp = false;
		}
		else
		{
			// Update this players actions
			EntityManager::Get().Collect<NetworkPlayerComponent, ThisPlayer, InputController>().Do([&](NetworkPlayerComponent&, ThisPlayer&, InputController& inputC)
				{

					m_playerInputUdp.shoot = inputC.shoot;
					m_playerInputUdp.jump = inputC.jump;
					m_playerInputUdp.activateActiveItem = inputC.activateActiveItem;
					m_playerInputUdp.switchComp = inputC.switchComp;

				});
			//Update the others players
			EntityManager::Get().Collect<TransformComponent, NetworkPlayerComponent, InputController, OnlinePlayer>().Do([&](TransformComponent& transformC, NetworkPlayerComponent& networkC, InputController& inputC, OnlinePlayer&)
				{
					transformC.worldMatrix = m_outputUdp.m_holdplayersUdp[networkC.playerId].matrix;
					transformC.SetScale(DirectX::SimpleMath::Vector3(0.5f, 0.5f, 0.5f));
					inputC.shoot = m_outputUdp.m_holdplayersUdp[networkC.playerId].shoot;
					inputC.jump = m_outputUdp.m_holdplayersUdp[networkC.playerId].jump;
					inputC.activateActiveItem = m_outputUdp.m_holdplayersUdp[networkC.playerId].activateActiveItem;
					inputC.switchComp = m_outputUdp.m_holdplayersUdp[networkC.playerId].switchComp;
				});

			// Sync the rest
			if (m_dataIsReadyToBeSentTcp == false)
			{
				m_inputTcp.nrOfNetTransform = 0;
				m_inputTcp.nrOfNetStats = 0;
				m_inputTcp.nrOfCreateAndDestroy = 0;
				if (m_inputTcp.playerId > -1)
				{
					m_bufferSize += sizeof(Client::ClientsData);
					//sync all transforms Host only
					if (m_inputTcp.playerId == 0)
					{
						EntityManager::Get().Collect<NetworkTransform, TransformComponent>().Do([&](entity id, NetworkTransform& netC, TransformComponent& transC)
							{
								netC.objectId = id;
								netC.transform = transC.worldMatrix;
								memcpy(m_sendBuffer + m_bufferSize, &netC, sizeof(NetworkTransform));
								m_inputTcp.nrOfNetTransform++;
								m_bufferSize += sizeof(NetworkTransform);

							});
					}

					EntityManager::Get().Collect<NetworkAgentStats, AgentStatsComponent>().Do([&](entity id, NetworkAgentStats& netC, AgentStatsComponent& AgentS)
						{
							netC.objectId = id; // replace with enemy id
							netC.stats = AgentS;
							memcpy(m_sendBuffer + m_bufferSize, &netC, sizeof(NetworkAgentStats));
							m_inputTcp.nrOfNetStats++;
							m_bufferSize += sizeof(NetworkAgentStats);

						});

					EntityManager::Get().Collect<CreateAndDestroyEntityComponent>().Do([&](entity id, CreateAndDestroyEntityComponent& cdC)
						{
							cdC.playerId = m_inputTcp.playerId;
							memcpy(m_sendBuffer + m_bufferSize, &cdC, sizeof(CreateAndDestroyEntityComponent));
							m_bufferSize += sizeof(CreateAndDestroyEntityComponent);
							m_entityManager.RemoveComponent<CreateAndDestroyEntityComponent>(id);
							m_inputTcp.nrOfCreateAndDestroy++;
						});

					m_dataIsReadyToBeSentTcp = true;
				}
				// Recived data
				if (m_dataIsReadyToBeReceivedTcp)
				{
					memcpy(m_outputTcp, m_receiveBuffer, sizeof(Client::ClientsData) * MAX_PLAYER_COUNT);
					m_bufferReceiveSize += sizeof(Client::ClientsData) * MAX_PLAYER_COUNT;
					if (m_outputTcp->nrOfNetTransform > 0 && m_outputTcp->playerId < MAX_PLAYER_COUNT)
					{
						//Update the transfroms, Only none hosts
						if (m_inputTcp.playerId > 0)
						{
							NetworkTransform* tempTransfrom = new NetworkTransform;
							EntityManager::Get().Collect<NetworkTransform, TransformComponent>().Do([&](entity id, NetworkTransform&, TransformComponent& transC)
								{
									for (int i = 0; i < m_outputTcp[0].nrOfNetTransform; ++i)
									{
										//todo make better
										memcpy(tempTransfrom, m_receiveBuffer + m_bufferReceiveSize + sizeof(NetworkTransform) * i, sizeof(NetworkTransform));
										if (id == tempTransfrom->objectId)
										{
											transC.worldMatrix = tempTransfrom->transform;
										}

									}

								});
							delete tempTransfrom;
						}
						m_bufferReceiveSize += m_outputTcp->nrOfNetTransform * sizeof(NetworkTransform);
					}

					if (m_outputTcp->nrOfNetStats > 0)
					{
						NetworkAgentStats* tempStats = new NetworkAgentStats;
						EntityManager::Get().Collect<NetworkAgentStats, AgentStatsComponent>().Do([&](entity id, NetworkAgentStats&, AgentStatsComponent& Agent)
							{
								for (int i = 0; i < m_outputTcp[0].nrOfNetStats; ++i)
								{
									memcpy(tempStats, m_receiveBuffer + m_bufferReceiveSize + sizeof(NetworkAgentStats) * i, sizeof(NetworkAgentStats));
									if (id == tempStats->objectId)
									{
										Agent = tempStats->stats;
									}

								}
							});
						m_bufferReceiveSize += sizeof(NetworkAgentStats) * m_outputTcp->nrOfNetTransform;
						delete tempStats;
					}

					if (m_outputTcp->nrOfCreateAndDestroy > 0)
					{
						CreateAndDestroyEntityComponent* tempCreate = new CreateAndDestroyEntityComponent;
						for (int i = 0; i < m_outputTcp[0].nrOfNetStats; ++i)
						{
							memcpy(tempCreate, m_receiveBuffer + m_bufferReceiveSize + sizeof(CreateAndDestroyEntityComponent) * i, sizeof(CreateAndDestroyEntityComponent));
							if (tempCreate->playerId != m_inputTcp.playerId)
							{
								if (tempCreate->alive)
								{
									std::cout << "Created entity of type: " << tempCreate->entityTypeId << " With id: " << tempCreate->id << "From player: " << tempCreate->playerId
										<< std::endl;
									//send to correct entity type spawner 
								}
								else
								{
									std::cout << "Destroyed entity of type: " << tempCreate->entityTypeId << " With id: " << tempCreate->id << "From player: " << tempCreate->playerId
										<< std::endl;
									//send to correct entity type destroyer
								}
							}
						}
						delete tempCreate;

					}

					//reset recived bufferSize
					m_bufferReceiveSize = 0;
					m_dataIsReadyToBeReceivedTcp = false;
				}
			}
		}
	}
}

void NetCode::Receive()
{
	m_threadUdp = std::thread(&NetCode::ReceiveUdp, this);
	while (m_startUp)
		continue;
	if (m_netCodeAlive)
	{

		//tcp
		
		while (m_netCodeAlive)
		{
  			if (m_dataIsReadyToBeSentTcp)
			{
				memcpy(m_sendBuffer, (char*)&m_inputTcp, sizeof(m_inputTcp));
				m_client.SendChararrayTcp(m_sendBuffer, m_bufferSize);
				m_receiveBuffer = m_client.ReceiveCharArrayTcp(m_receiveBuffer); // todo put in own thread
				if (m_receiveBuffer == nullptr)
				{
					std::cout << "Bad tcp packet \n";
				}
				else
				{
					m_dataIsReadyToBeReceivedTcp = true;
				}
				m_bufferSize = 0;
				m_dataIsReadyToBeSentTcp = false;
			}
		}
			
		}
}
	
void NetCode::ReceiveUdp()
{
	m_playerInputUdp.playerId = m_inputTcp.playerId;
	while (m_netCodeAlive)
	{
		m_mut.lock();
		m_client.SendUdp(m_playerInputUdp);
		m_mut.unlock();
		m_outputUdp = m_client.ReceiveUdp();
	}

}

void NetCode::AddMatrixUdp(DirectX::XMMATRIX input)
{
	m_mut.lock();
	m_playerInputUdp.matrix = input;
	m_mut.unlock();
}

bool NetCode::Host()
{
	
	bool server = m_serverHost.StartTcpServer();
	if (server)
	{
		// join server
		std::string ip = m_serverHost.GetIpAddress();
		if (ip != "")
		{
			std::cout << "Hosting at: " << ip << std::endl;
			m_inputTcp.playerId = m_client.ConnectTcpServer(ip);
			if (m_inputTcp.playerId > -1)
			{
				m_client.SendTcp(m_inputTcp);
				m_outputTcp = m_client.ReceiveTcp();
				if (m_active == false)
				{
					m_startUp = true;
					m_active = true;

				}
				m_thread = std::thread(&NetCode::Receive, this);
				return server;
			}
		}
	}
	return server;
}

bool NetCode::Join(char* inputString)
{
	if (inputString[0] == 'd')
	{
		m_inputTcp.playerId = m_client.ConnectTcpServer("192.168.1.72"); //192.168.1.55 || 192.168.50.214
	}
	else
	{
		m_inputTcp.playerId = m_client.ConnectTcpServer(inputString);
	}

	if (m_inputTcp.playerId > -1)
	{
		m_outputTcp = m_client.ReceiveTcp();		
		if (m_active == false)
		{
			m_startUp = true;
			m_active = true;

		}
		m_thread = std::thread(&NetCode::Receive, this);
		return true;
	}
	return false;
}

INT8 NetCode::Play()
{
	if (m_active)
		return m_serverHost.GetNrOfConnectedPlayers();
	else if (m_inputTcp.playerId == 0)
		return 1;
	else
		std::cout << "Beep boop You are not the host, please dont press Play it hurts" << std::endl;
	return -1;
}