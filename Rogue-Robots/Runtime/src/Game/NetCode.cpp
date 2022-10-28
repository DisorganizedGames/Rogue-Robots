#include "NetCode.h"
using namespace DOG;
NetCode::NetCode()
{


	m_netCodeAlive = TRUE;
	m_inputTcp.lobbyAlive = true;
	m_playerInputUdp.playerId = 0;
	m_playerInputUdp.playerTransform = {};


	m_hardSyncTcp = FALSE;
	m_active = FALSE;
	m_startUp = FALSE;
	

	m_bufferSize = 0;
	m_bufferReceiveSize = 0;
	m_receiveBuffer = new char[SEND_AND_RECIVE_BUFFER_SIZE];
	m_dataIsReadyToBeReceivedTcp = false;
	m_lobby = false;
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

void NetCode::OnStartup()
{
	DOG::EntityManager& m_entityManager = DOG::EntityManager::Get();
	EntityManager::Get().Collect<NetworkPlayerComponent, ThisPlayer, TransformComponent>().Do([&](entity id, NetworkPlayerComponent& networkC, ThisPlayer&, TransformComponent&)
		{
			if (networkC.playerId != m_inputTcp.playerId)
			{
				m_entityManager.AddComponent<OnlinePlayer>(id);
				m_entityManager.RemoveComponent<ThisPlayer>(id);
				m_entityManager.RemoveComponent<AudioListenerComponent>(id);
			}

		});
	EntityManager::Get().Collect<NetworkPlayerComponent, TransformComponent, OnlinePlayer>().Do([&](entity id, NetworkPlayerComponent& networkC, TransformComponent&, OnlinePlayer&)
		{
			if (networkC.playerId == m_inputTcp.playerId)
			{
				m_entityManager.AddComponent<ThisPlayer>(id);
				m_entityManager.AddComponent<AudioListenerComponent>(id);
				m_entityManager.RemoveComponent<OnlinePlayer>(id);
			}
		});

}

void NetCode::OnUpdate(AgentManager* agentManager)
{
	UpdateSendUdp();

	if (m_active)
	{
		DOG::EntityManager& m_entityManager = DOG::EntityManager::Get();
		//UDP /////////////////////////////////////////////////////////////////////
		// Update this players actions
		EntityManager::Get().Collect<NetworkPlayerComponent, ThisPlayer, InputController>().Do([&](NetworkPlayerComponent&, ThisPlayer&, InputController& inputC)
			{

				m_playerInputUdp.actions = inputC;
			});
		//Update the others players
		EntityManager::Get().Collect<TransformComponent, NetworkPlayerComponent, InputController, OnlinePlayer, PlayerStatsComponent
		>().Do([&](TransformComponent& transformC, NetworkPlayerComponent& networkC, InputController& inputC, OnlinePlayer&, PlayerStatsComponent& statsC)
			{
				transformC.worldMatrix = m_outputUdp.m_holdplayersUdp[networkC.playerId].playerTransform;
				transformC.SetScale(DirectX::SimpleMath::Vector3(1.0f, 1.0f, 1.0f));
				inputC = m_outputUdp.m_holdplayersUdp[networkC.playerId].actions;
				statsC = m_outputUdp.m_holdplayersUdp[networkC.playerId].playerStat;

			});
		//Tcp////////////////////////////////////////////////////////////////////////
			// Collect data to send
			m_inputTcp.nrOfNetTransform = 0;
			m_inputTcp.nrOfChangedAgentsHp = 0;
			m_inputTcp.nrOfCreateAndDestroy = 0;

			//check if player has valid id
			if (m_inputTcp.playerId > -1)
			{
				m_bufferSize += sizeof(ClientsData);
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

				EntityManager::Get().Collect<NetworkAgentStats, AgentHPComponent, AgentIdComponent>().Do([&](NetworkAgentStats& netC, AgentHPComponent& agentS, AgentIdComponent& idC)
					{
						if (agentS.damageThisFrame)
						{
							agentS.damageThisFrame = false;
							netC.playerId = m_inputTcp.playerId;
							netC.objectId = idC.id;
							netC.hp = agentS;
							memcpy(m_sendBuffer + m_bufferSize, &netC, sizeof(NetworkAgentStats));
							m_inputTcp.nrOfChangedAgentsHp++;
							m_bufferSize += sizeof(NetworkAgentStats);
						}
					});

				EntityManager::Get().Collect<CreateAndDestroyEntityComponent>().Do([&](entity id, CreateAndDestroyEntityComponent& cdC)
					{
						cdC.playerId = m_inputTcp.playerId;
						memcpy(m_sendBuffer + m_bufferSize, &cdC, sizeof(CreateAndDestroyEntityComponent));
						m_bufferSize += sizeof(CreateAndDestroyEntityComponent);
						m_entityManager.RemoveComponent<CreateAndDestroyEntityComponent>(id);
						m_inputTcp.nrOfCreateAndDestroy++;
					});

				m_inputTcp.sizeOfPayload = m_bufferSize;
				memcpy(m_sendBuffer, (char*)&m_inputTcp, sizeof(m_inputTcp));
				m_client.SendChararrayTcp(m_sendBuffer, m_bufferSize);
				m_bufferSize = 0;
			}
			
			// Recived data
			while(m_numberOfPackets > 0 && m_dataIsReadyToBeReceivedTcp)
				{
					//Get the header
					TcpHeader header;
					memcpy(&header, m_receiveBuffer+ m_bufferReceiveSize, sizeof(TcpHeader));
					m_bufferReceiveSize += sizeof(TcpHeader);
					if (header.playerId > MAX_PLAYER_COUNT || header.playerId < 0)
					{
						std::cout << "Error: header is corrupt: " << std::endl;
						m_numberOfPackets = 0;
						m_dataIsReadyToBeReceivedTcp = false;
					}
					else
					{
						//update
						m_inputTcp.nrOfPlayersConnected = header.nrOfPlayersConnected;
						if (m_inputTcp.playerId > 0)
							m_inputTcp.lobbyAlive = header.lobbyAlive;


						if (header.nrOfNetTransform > 0 && header.playerId < MAX_PLAYER_COUNT)
						{
							//Update the transfroms, Only none hosts
							if (m_inputTcp.playerId > 0)
							{
								NetworkTransform* tempTransfrom = new NetworkTransform;
								EntityManager::Get().Collect<NetworkTransform, TransformComponent, AgentIdComponent>().Do([&](NetworkTransform&, TransformComponent& transC, AgentIdComponent& idC)
									{
										for (u32 i = 0; i < header.nrOfNetTransform; ++i)
										{
											//todo make better
											memcpy(tempTransfrom, m_receiveBuffer + m_bufferReceiveSize + sizeof(NetworkTransform) * i, sizeof(NetworkTransform));
											if (idC.id == tempTransfrom->objectId)
											{
												transC.worldMatrix = tempTransfrom->transform;
											}

										}

									});
								delete tempTransfrom;
							}
							m_bufferReceiveSize += header.nrOfNetTransform * sizeof(NetworkTransform);
						}
 

						if (header.nrOfChangedAgentsHp > 0)
						{
							NetworkAgentStats* tempStats = new NetworkAgentStats;
							EntityManager::Get().Collect<NetworkAgentStats, AgentHPComponent, AgentIdComponent>().Do([&](NetworkAgentStats&, AgentHPComponent& Agent, AgentIdComponent& idC)
								{
									for (u32 i = 0; i < header.nrOfChangedAgentsHp; ++i)
									{
										memcpy(tempStats, m_receiveBuffer + m_bufferReceiveSize + sizeof(NetworkAgentStats) * i, sizeof(NetworkAgentStats));
										if (idC.id == tempStats->objectId && tempStats->hp.hp < Agent.hp)
										{
											Agent = tempStats->hp;
										}

									}
								});
							m_bufferReceiveSize += sizeof(NetworkAgentStats) * header.nrOfChangedAgentsHp;
							delete tempStats;
						}

						if (header.nrOfCreateAndDestroy > 0)
						{
							CreateAndDestroyEntityComponent* tempCreate = new CreateAndDestroyEntityComponent;
							for (u32 i = 0; i < header.nrOfCreateAndDestroy; ++i)
							{
								memcpy(tempCreate, m_receiveBuffer + m_bufferReceiveSize + sizeof(CreateAndDestroyEntityComponent) * i, sizeof(CreateAndDestroyEntityComponent));
								if (tempCreate->playerId != m_inputTcp.playerId)
								{
									EntityManager::Get().Collect<AgentIdComponent>().Do([&](AgentIdComponent&)
										{
											if ((u32)tempCreate->entityTypeId < (u32)EntityTypes::Agents && !tempCreate->alive)
											{
												agentManager->CreateOrDestroyShadowAgent(*tempCreate);
											}
										});
								}
							}
							m_bufferReceiveSize += sizeof(CreateAndDestroyEntityComponent) * header.nrOfCreateAndDestroy;
							delete tempCreate;

						}
					}
					m_numberOfPackets--;
					
				}
			//reset recived bufferSize
			m_bufferReceiveSize = 0;
			m_dataIsReadyToBeReceivedTcp = false;
			}
}


void NetCode::Receive()
{
	//Game loop
	m_threadUdp = std::thread(&NetCode::ReceiveUdp, this);
	if (m_netCodeAlive)
	{
		//tcp
		m_active = true;
		m_startUp = true;
		while (m_netCodeAlive)
		{
			while (m_dataIsReadyToBeReceivedTcp && m_netCodeAlive)
				continue;
			
			m_numberOfPackets = m_client.ReceiveCharArrayTcp(m_receiveBuffer);
		if (m_receiveBuffer == nullptr)
		{
			std::cout << "Bad tcp packet \n";
		}
		else
		{
			m_dataIsReadyToBeReceivedTcp = true;
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

void NetCode::UpdateSendUdp()
{
	EntityManager::Get().Collect<ThisPlayer, TransformComponent, PlayerStatsComponent, InputController>().Do([&](
		ThisPlayer&, TransformComponent& transC, PlayerStatsComponent& statsC, InputController& inputC)
		{
			m_playerInputUdp.playerTransform = transC.worldMatrix;
			m_playerInputUdp.playerStat = statsC;
			m_playerInputUdp.actions = inputC;
		});
}

void NetCode::AddMatrixUdp(DirectX::XMMATRIX input)
{
	m_mut.lock();
	m_playerInputUdp.playerTransform = input;
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
				m_inputTcp.sizeOfPayload = sizeof(m_inputTcp);
				m_inputTcp.lobbyAlive = true;
				//m_client.SendTcp(m_inputTcp); // check if client needs to
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
		m_inputTcp.playerId = m_client.ConnectTcpServer("192.168.1.74"); //192.168.1.55 || 192.168.50.214
	}
	else
	{
		m_inputTcp.playerId = m_client.ConnectTcpServer(inputString);
	}

	if (m_inputTcp.playerId > -1)
	{
		m_inputTcp.lobbyAlive = true;
		m_thread = std::thread(&NetCode::Receive, this);
		return true;
	}
	return false;
}

INT8 NetCode::Play()
{
	m_inputTcp.lobbyAlive = false;
	return MAX_PLAYER_COUNT;
}

u8 NetCode::GetNrOfPlayers()
{
	return m_inputTcp.nrOfPlayersConnected;
}

//host only
std::string NetCode::GetIpAdress()
{
	return m_serverHost.GetIpAddress();
}

bool NetCode::IsLobbyAlive()
{
	return m_inputTcp.lobbyAlive;
}