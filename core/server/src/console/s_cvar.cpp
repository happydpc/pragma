#include "stdafx_server.h"
#include "pragma/console/s_cvar.h"
#include "pragma/console/convarhandle.h"
#include <pragma/serverstate/serverstate.h>

extern ServerState *server;
ConVarHandle GetServerConVar(std::string scmd)
{
	return server->GetConVarHandle(scmd);
}