#pragma once
#include <DOGEngine.h>
#include "..\Game\GameComponent.h"

constexpr int SEND_AND_RECIVE_BUFFER_SIZE = 16384;
constexpr int MAX_PLAYER_COUNT = 4;
constexpr const char* PORTNUMBER_OUT = "50005";
constexpr const char* PORTNUMBER_IN = "50004";
constexpr int PORTNUMBER_OUT_INT = 50006;
constexpr int PORTNUMBER_IN_INT = 50004;
constexpr const char* MULTICAST_ADRESS = "239.255.255.0";

struct PlayerNetworkComponentUdp
{
	i8 playerId = 0;
	u64 udpId = 0;
	DirectX::XMMATRIX playerTransform = {};
	InputController actions;
	PlayerStatsComponent playerStat;
	DirectX::XMMATRIX cameraTransform = {};
};

struct UdpData
{
	int nrOfEntites;
	u64 udpId;

};

struct UdpReturnData
{
	PlayerNetworkComponentUdp m_holdplayersUdp[MAX_PLAYER_COUNT];
};

struct TcpHeader
{
	i8 playerId = 0;
	i8 nrOfPlayersConnected = 0;
	u16 sizeOfPayload = 0;
	u16 nrOfNetTransform = 0;
	u16 nrOfChangedAgentsHp = 0;
	u16 nrOfCreateAndDestroy = 0;
	bool lobbyAlive = true;

};

struct ClientsData
{
	i8 playerId = 0;
	u8 nrOfPlayersConnected = 0;
	u16 sizeOfPayload = 0;
	u16 nrOfNetTransform = 0;
	u16 nrOfChangedAgentsHp = 0;
	u16 nrOfCreateAndDestroy = 0;
	bool lobbyAlive = false;
};