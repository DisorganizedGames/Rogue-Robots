#include "NetCode.h"
//todo
// exclude entities with thisplayerComponent
// Swap models
using namespace DOG;
NetCode::NetCode()
{


	m_netCodeAlive = TRUE;
	m_outputTcp = nullptr;
	m_inputTcp.matrix = {};

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

	if (m_active)
	{

		if (m_startUp == TRUE)
		{

			EntityManager::Get().Collect<NetworkPlayerComponent, ThisPlayer, TransformComponent, ModelComponent>().Do([&](NetworkPlayerComponent& networkC, ThisPlayer&, TransformComponent& transC, ModelComponent&)
				{
					transC.worldMatrix = m_outputTcp[networkC.playerId].matrix;
					networkC.playerId = m_inputTcp.playerId;
					//modelC.id = ;

				});
			EntityManager::Get().Collect<NetworkPlayerComponent, TransformComponent, ModelComponent>().Do([&](NetworkPlayerComponent& networkC, TransformComponent& transC, ModelComponent&)
				{
					if (networkC.playerId == m_inputTcp.playerId) // todo
					{
						networkC.playerId = 0;
						//modelC.id = ;
					}
					transC.worldMatrix = m_outputTcp[networkC.playerId].matrix;
				});

			m_startUp = false;
		}

		EntityManager::Get().Collect<ThisPlayer, TransformComponent>().Do([&](ThisPlayer&, TransformComponent& transC)
			{
				AddMatrixTcp(transC.worldMatrix);
				AddMatrixUdp(transC.worldMatrix);
			});

		EntityManager::Get().Collect<TransformComponent, NetworkPlayerComponent, InputController, OnlinePlayer>().Do([&](TransformComponent& transformC, NetworkPlayerComponent& networkC, InputController& inputC, OnlinePlayer&)
			{
				transformC.worldMatrix = m_outputUdp.m_holdplayersUdp[networkC.playerId].matrix;
				inputC.shoot = m_outputUdp.m_holdplayersUdp[networkC.playerId].shoot;
				inputC.jump = m_outputUdp.m_holdplayersUdp[networkC.playerId].jump;
				inputC.activateActiveItem = m_outputUdp.m_holdplayersUdp[networkC.playerId].activateActiveItem;
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
			bool server = serverTest.StartTcpServer();
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
			m_client.SendTcp(m_inputTcp);
			m_mut.unlock();
			m_outputTcp = m_client.ReciveTcp();
		}
	}
}

void NetCode::ReciveUdp()
{
	while (m_netCodeAlive)
	{
		m_mut.lock();
		m_client.SendUdp(m_playerInputUdp);
		m_mut.unlock();
		m_outputUdp = m_client.ReciveUdp();
	}

}

void NetCode::AddPlayersId(std::vector<DOG::entity> playersId)
{
	m_playersId = playersId;
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