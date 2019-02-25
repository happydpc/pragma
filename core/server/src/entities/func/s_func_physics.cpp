#include "stdafx_server.h"
#include "pragma/entities/func/s_func_physics.h"
#include "pragma/entities/s_entityfactories.h"
#include "pragma/physics/movetypes.h"
#include <pragma/physics/physobj.h>
#include <sharedutils/util_string.h>
#include <sharedutils/util.h>
#include <pragma/networking/nwm_util.h>
#include "pragma/lua/s_lentity_handles.hpp"

using namespace pragma;

LINK_ENTITY_TO_CLASS(func_physics,FuncPhysics);

void SFuncPhysicsComponent::Initialize()
{
	BaseFuncPhysicsComponent::Initialize();
	if(m_bClientsidePhysics == true)
		static_cast<SBaseEntity&>(GetEntity()).SetSynchronized(false);
}

void SFuncPhysicsComponent::SendData(NetPacket &packet,nwm::RecipientFilter &rp)
{
	packet->Write<float>(m_kvMass);
	packet->WriteString(m_kvSurfaceMaterial);
}

PhysObj *SFuncPhysicsComponent::InitializePhysics()
{
	if(m_bClientsidePhysics == true)
		return nullptr;
	return BaseFuncPhysicsComponent::InitializePhysics();
}

/////////////

luabind::object SFuncPhysicsComponent::InitializeLuaObject(lua_State *l) {return BaseEntityComponent::InitializeLuaObject<SFuncPhysicsComponentHandleWrapper>(l);}

void FuncPhysics::Initialize()
{
	SBaseEntity::Initialize();
	AddComponent<SFuncPhysicsComponent>();
}