#ifndef __C_GAME_AUX_HPP__
#define __C_GAME_AUX_HPP__

#include "pragma/clientdefinitions.h"
#include "pragma/game/c_game.h"

extern DLLCENGINE CEngine *c_engine;

template<class TEfxProperties>
	std::shared_ptr<al::Effect> CGame::CreateAuxEffect(const std::string &name,const TEfxProperties &props)
{
	return c_engine->CreateAuxEffect(name,props);
}

#endif