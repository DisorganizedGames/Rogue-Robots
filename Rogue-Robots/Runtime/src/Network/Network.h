#pragma once
#include <DOGEngine.h>
#include "..\Game\GameComponent.h"
constexpr float TICKRATE = 1.0f / 60.0f /*1.0f/24.0f*/ /*1.0f/120.0f*/ /*1.0f/5.0f*/ /*1.0f/1000.0f*/;
constexpr int SEND_AND_RECIVE_BUFFER_SIZE = 262144;
constexpr int MAX_PLAYER_COUNT = 4;
constexpr const char* PORTNUMBER_OUT = "50005";
constexpr const char* PORTNUMBER_IN = "50004";
constexpr int PORTNUMBER_OUT_INT = 50006;
constexpr int PORTNUMBER_IN_INT = 50004;
constexpr const char* MULTICAST_ADRESS = "239.255.255.0"; //Default multicast
constexpr u32 AGGRO_BIT = 2147483648;
constexpr f32 TEAM_DAMAGE_MODIFIER = 12.0f; //At 1.0f it does orginal damage, higher value deal less damage
constexpr int HARD_SYNC_FRAME = 30;

struct PlayerNetworkComponentUdp
{
	i8 playerId = 0;
	u64 udpId = 0;
	DirectX::SimpleMath::Matrix playerTransform = {};
	InputController actions;
	PlayerStatsComponent playerStat{};
	DirectX::SimpleMath::Matrix cameraTransform = {};
};

struct UdpData
{
	int nrOfEntites  = 0;
	u64 udpId = 0;

};

struct UdpReturnData
{
	PlayerNetworkComponentUdp m_holdplayersUdp[MAX_PLAYER_COUNT];
};

struct TcpHeader
{
	i8 playerId = 0;
	u16 sizeOfPayload = 0;
	u16 nrOfNetTransform = 0;
	u16 nrOfChangedAgentsHp = 0;
	u16 nrOfCreateAndDestroy = 0;
	bool lobbyAlive = true;
	u16 nrOfPathFindingSync = 0;
};

struct NetworkId
{
	EntityTypes entityTypeId = EntityTypes::Default;
	u32 id = u32(-1);
};

struct PathFindingSync
{
	AgentIdComponent id = { 0, EntityTypes::Default};
};

struct LobbyData
{
	u8 nrOfPlayersConnected = 1;
	bool playersSlotConnected[MAX_PLAYER_COUNT] = {true, false, false, false};
	u16 levelIndex = 0;
	char data[4096];
	u32 levelSize = 0;
	u32 levelDataIndex = 0;
};