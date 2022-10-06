#include "NetCode.h"
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
						transC.worldMatrix = m_outputTcp[networkC.playerId].matrix;
					}

				});
			EntityManager::Get().Collect<NetworkPlayerComponent, TransformComponent, OnlinePlayer>().Do([&](entity id, NetworkPlayerComponent& networkC, TransformComponent& transC, OnlinePlayer&)
				{
					if (networkC.playerId == m_inputTcp.playerId)
					{
						m_entityManager.AddComponent<ThisPlayer>(id);
						m_entityManager.AddComponent<CameraComponent>(id);
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
		{
			std::cout << "Failed input, try agiain" << std::endl;
		//fseek(stdin, 0, SEEK_END);
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
	m_playerInputUdp.playerId = m_inputTcp.playerId;
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