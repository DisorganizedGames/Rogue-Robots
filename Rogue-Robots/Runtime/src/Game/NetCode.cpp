#include "NetCode.h"
#include "ItemManager/ItemManager.h"

using namespace DOG;
NetCode::NetCode()
{
	m_netCodeAlive = true;
	m_inputTcp.lobbyAlive = true;
	m_playerInputUdp.playerId = 0;
	m_playerInputUdp.playerTransform = {};
	m_inputTcp.nrOfChangedAgentsHp = 0;
	m_inputTcp.nrOfCreateAndDestroy = 0;
	m_inputTcp.nrOfNetTransform = 0;
	m_hardSyncTcp = false;
	m_active = false;
	m_startUp = false;
	
	m_bufferSize = sizeof(ClientsData);;
	m_bufferReceiveSize = 0;
	m_receiveBuffer = new char[SEND_AND_RECIVE_BUFFER_SIZE];
	m_dataIsReadyToBeReceivedTcp = false;
	m_lobby = false;
	//Tick
	QueryPerformanceFrequency(&m_clockFrequency);
	QueryPerformanceCounter(&m_tickStartTime);
	m_sleepGranularityMs = 1;
}

NetCode::~NetCode()
{
	m_netCodeAlive = false;
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
				EntityManager::Get().Collect<DontDraw, ChildComponent>().Do([&](entity subEntity, DontDraw&, ChildComponent& parentCompany)
					{
						if (parentCompany.parent == id)
						{
							m_entityManager.RemoveComponent<DontDraw>(subEntity);
						}
					});
				m_entityManager.RemoveComponent<AudioListenerComponent>(id);
			}

		});
	EntityManager::Get().Collect<NetworkPlayerComponent, TransformComponent, OnlinePlayer>().Do([&](entity id, NetworkPlayerComponent& networkC, TransformComponent&, OnlinePlayer&)
		{
			if (networkC.playerId == m_inputTcp.playerId)
			{
				EntityManager::Get().Collect<ChildComponent>().Do([&](entity subEntity, ChildComponent& parentC)
					{
						if (parentC.parent == id)
						{
							m_entityManager.AddComponent<DontDraw>(subEntity);
						}
					});
				m_entityManager.AddComponent<ThisPlayer>(id);
				m_entityManager.AddComponent<AudioListenerComponent>(id);
				m_entityManager.RemoveComponent<OnlinePlayer>(id);
			}
		});

}

void NetCode::OnUpdate()
{
	UpdateSendUdp();

	if (m_active)
	{
		DOG::EntityManager& m_entityManager = DOG::EntityManager::Get();
		//UDP /////////////////////////////////////////////////////////////////////
		//Update the others players
		EntityManager::Get().Collect<TransformComponent, NetworkPlayerComponent, InputController, OnlinePlayer, PlayerStatsComponent, PlayerControllerComponent
		>().Do([&](entity id, TransformComponent& transformC, NetworkPlayerComponent& networkC, InputController& inputC, OnlinePlayer&, PlayerStatsComponent& statsC, PlayerControllerComponent& pC)
			{
				transformC.worldMatrix = m_outputUdp.m_holdplayersUdp[networkC.playerId].playerTransform;
				inputC = m_outputUdp.m_holdplayersUdp[networkC.playerId].actions;
				statsC = m_outputUdp.m_holdplayersUdp[networkC.playerId].playerStat;
				if (statsC.health > 0 && !m_entityManager.HasComponent<PlayerAliveComponent>(id))
					m_entityManager.AddComponent<PlayerAliveComponent>(id);
				if ((pC.cameraEntity != DOG::NULL_ENTITY) && (m_outputUdp.m_holdplayersUdp[networkC.playerId].cameraTransform.Determinant() != 0)) {
					m_entityManager.GetComponent<TransformComponent>(pC.cameraEntity).worldMatrix = m_outputUdp.m_holdplayersUdp[networkC.playerId].cameraTransform;
				}
				});
		//Tcp////////////////////////////////////////////////////////////////////////
			// Collect data to send



			//check if player has valid id
			if (m_inputTcp.playerId > -1)
			{
				
				//sync all transforms Host only
		/*		if (m_inputTcp.playerId == 0)
				{
					EntityManager::Get().Collect<NetworkTransform, TransformComponent, AgentIdComponent>().Do([&](NetworkTransform& netC, TransformComponent& transC, AgentIdComponent agentId)
						{
							netC.objectId = agentId.id;
							netC.transform = transC.worldMatrix;
							memcpy(m_sendBuffer + m_bufferSize, &netC, sizeof(NetworkTransform));
							m_inputTcp.nrOfNetTransform++;
							m_bufferSize += sizeof(NetworkTransform);

						});
				}*/

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
				if (Server::TickTimeLeftTCP(m_tickStartTime, m_clockFrequency) > (1.0f / 30.0f))
				{
					m_inputTcp.sizeOfPayload = m_bufferSize;
					memcpy(m_sendBuffer, (char*)&m_inputTcp, sizeof(ClientsData));
					m_client.SendChararrayTcp(m_sendBuffer, m_inputTcp.sizeOfPayload);
					m_bufferSize = sizeof(ClientsData);
					m_inputTcp.nrOfNetTransform = 0;
					m_inputTcp.nrOfChangedAgentsHp = 0;
					m_inputTcp.nrOfCreateAndDestroy = 0;
					QueryPerformanceCounter(&m_tickStartTime);
				}
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
								
								if ((u32)tempCreate->entityTypeId < (u32)EntityTypes::Agents && !tempCreate->alive)
								{
									AgentManager::Get().CreateOrDestroyShadowAgent(*tempCreate);
								}
					
								//Destroy pickups
								if ((u32)tempCreate->entityTypeId < (u32)EntityTypes::Magazines && !tempCreate->alive && (u32)tempCreate->entityTypeId > (u32)EntityTypes::Agents )
								{
									EntityManager::Get().Collect<NetworkPlayerComponent, PlayerAliveComponent>().Do([&](entity id, NetworkPlayerComponent& playerC, PlayerAliveComponent&)
										{
											if (playerC.playerId == tempCreate->playerId)
											{
														
												EntityManager::Get().Collect<NetworkId>().Do([&](entity e, NetworkId& nIdC)
													{
																	
														if (nIdC.entityTypeId == tempCreate->entityTypeId && nIdC.id == tempCreate->id)
														{
															std::string luaEventName = std::string("ItemPickup") + std::to_string(id);
															DOG::LuaMain::GetEventSystem()->InvokeEvent(luaEventName, (u32)tempCreate->entityTypeId);
															m_entityManager.RemoveComponent<NetworkId>(e);
															m_entityManager.DeferredEntityDestruction(e);
														}
													});
															
											}
										});

								}
								
								//Create pickups
								if ((u32)tempCreate->entityTypeId < (u32)EntityTypes::Magazines && tempCreate->alive && (u32)tempCreate->entityTypeId >(u32)EntityTypes::Agents)
								{
									ItemManager::Get().CreateItemClient(*tempCreate);
								}
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
	bool firstTime = false;
	//Game loop
	
	if (m_netCodeAlive)
	{
		//tcp
		m_active = true;
		m_startUp = true;
		while (m_netCodeAlive)
		{
			while (m_dataIsReadyToBeReceivedTcp && m_netCodeAlive)
				continue;

			if (!m_netCodeAlive)
				break;
			if (!firstTime && !m_inputTcp.lobbyAlive)
			{
				firstTime = true;
				m_threadUdp = std::thread(&NetCode::ReceiveUdp, this);
				m_threadUdp.detach();
			}
			


			m_numberOfPackets = m_client.ReceiveCharArrayTcp(m_receiveBuffer);
			
			if (m_receiveBuffer == nullptr || m_numberOfPackets == 0)
			{
				std::cout << "NetCode:: Bad tcp packet, Number of packets: " << m_numberOfPackets << std::endl;
			}
			else
			{
				m_dataIsReadyToBeReceivedTcp = true;
			}
		}
			
	}
	std::cout << "Client: stopped reciving packets \n";
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
	m_mut.lock();
	EntityManager::Get().Collect<ThisPlayer, TransformComponent, PlayerStatsComponent, InputController, PlayerControllerComponent>().Do([&](
		ThisPlayer&, TransformComponent& transC, PlayerStatsComponent& statsC, InputController& inputC, PlayerControllerComponent& pC)
		{
			m_playerInputUdp.playerTransform = transC.worldMatrix;
			m_playerInputUdp.playerStat = statsC;
			m_playerInputUdp.actions = inputC;
			if (pC.cameraEntity != DOG::NULL_ENTITY)
			{
				DOG::EntityManager& entityManager = DOG::EntityManager::Get();
				if(entityManager.GetComponent<TransformComponent>(pC.cameraEntity).worldMatrix.Determinant() != 0)
					m_playerInputUdp.cameraTransform = entityManager.GetComponent<TransformComponent>(pC.cameraEntity).worldMatrix;
			}
		});
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
				m_thread.detach();
				return server;
			}
		}
	}
	return server;
}

bool NetCode::Join(char* inputString)
{
	if (inputString[0] == 'a')
		m_inputTcp.playerId = m_client.ConnectTcpServer("192.168.1.55"); //sam
	else if(inputString[0] == 'b')
		m_inputTcp.playerId = m_client.ConnectTcpServer("192.168.1.72"); // filip
	else if (inputString[0] == 'c')
		m_inputTcp.playerId = m_client.ConnectTcpServer("192.168.1.73"); // nad
	else if (inputString[0] == 'd')
		m_inputTcp.playerId = m_client.ConnectTcpServer("192.168.1.67"); // axel
	else if (inputString[0] == 'e')
		m_inputTcp.playerId = m_client.ConnectTcpServer("192.168.1.26"); //ove
	else if (inputString[0] == 'f')
		m_inputTcp.playerId = m_client.ConnectTcpServer("192.168.1.254"); //gunnar
	else if (inputString[0] == 'g')
		m_inputTcp.playerId = m_client.ConnectTcpServer("192.168.1.70"); // Emil
	else if (inputString[0] == 'h')
		m_inputTcp.playerId = m_client.ConnectTcpServer("192.168.1.76"); // Jonatan
	else if (inputString[0] == 'u')
	{
		m_inputTcp.playerId = m_client.ConnectTcpServer("192.168.1.55"); //192.168.1.55 || 192.168.50.214
	}
	else
	{
		m_inputTcp.playerId = m_client.ConnectTcpServer(inputString);
	}

	if (m_inputTcp.playerId > -1)
	{
		m_inputTcp.lobbyAlive = true;
		m_thread = std::thread(&NetCode::Receive, this);
		m_thread.detach();
		return true;
	}
	return false;
}

INT8 NetCode::Play()
{
	m_inputTcp.lobbyAlive = false;
	return GetNrOfPlayers();
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

void NetCode::SetLobbyStatus(bool lobbyStatus)
{
	m_inputTcp.lobbyAlive = lobbyStatus;
}

void NetCode::SetMulticastAdress(const char* adress)
{
	m_client.SetMulticastAdress(adress);
	m_serverHost.SetMulticastAdress(adress);
}

void DeleteNetworkSync::OnLateUpdate(DOG::entity e, DeferredDeletionComponent&, NetworkId& netId, TransformComponent& transC)
{
	DOG::EntityManager& m_entityManager = DOG::EntityManager::Get();
	entity newE = m_entityManager.CreateEntity();
	auto& t = m_entityManager.AddComponent<CreateAndDestroyEntityComponent>(newE);
	t.alive = false;
	t.entityTypeId = netId.entityTypeId;
	t.id = netId.id;
	t.position = transC.GetPosition();
	m_entityManager.RemoveComponent<NetworkId>(e);
}

