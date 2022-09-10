#pragma once
#include "Server.h"
#include "Client.h"
namespace DOG
{
	class NetCode
	{
	public:
		NetCode();
		~NetCode();

		void HostLobby(Server server);

		void JoinLobby();
	private:
		

	};
}