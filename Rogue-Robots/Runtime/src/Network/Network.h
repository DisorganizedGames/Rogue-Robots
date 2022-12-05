#pragma once
#include <DOGEngine.h>
#include "..\Game\GameComponent.h"

constexpr int SEND_AND_RECIVE_BUFFER_SIZE = 262144;
constexpr int MAX_PLAYER_COUNT = 4;
constexpr const char* PORTNUMBER_OUT = "50005";
constexpr const char* PORTNUMBER_IN = "50004";
constexpr int PORTNUMBER_OUT_INT = 50006;
constexpr int PORTNUMBER_IN_INT = 50004;
constexpr const char* MULTICAST_ADRESS = "239.255.255.0"; //Default multicast
constexpr u32 AGGRO_BIT = 2147483648;
constexpr f32 TEAM_DAMAGE_MODIFIER = 9.0f; //At 1.0f it does orginal damage, higher value deal less damage
constexpr int HARD_SYNC_FRAME = 60;
const std::string ROOM_1_IP = "192.168.1.55";
const std::string ROOM_2_IP = "192.168.1.72";
const std::string ROOM_3_IP = "192.168.1.73";
const std::string ROOM_4_IP = "192.168.1.67";
const std::string ROOM_5_IP = "192.168.1.26";
const std::string ROOM_6_IP = "192.168.1.254";
const std::string ROOM_7_IP = "192.168.1.70";
const std::string ROOM_8_IP = "192.168.1.76";
const std::string ROOM_9_IP = "192.168.1.8";

struct PlayerNetworkComponentUdp
{
	i8 playerId = 0;
	u64 udpId = 0;
	DirectX::SimpleMath::Matrix playerTransform = {};
	InputController actions;
	PlayerStatsComponent playerStat = {100.f, 0.0f, 5.5f, 0.f,11.0f };
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
	u8 nrOfPlayersConnected = 0;
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