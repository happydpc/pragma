#include "stdafx_server.h"
#include "pragma/entities/point/constraints/s_point_constraint_ballsocket.h"
#include "pragma/entities/s_entityfactories.h"
#include <pragma/physics/physobj.h>
#include <sharedutils/util_string.h>
#include <pragma/networking/nwm_util.h>
#include "pragma/lua/s_lentity_handles.hpp"
#include <pragma/entities/entity_component_system_t.hpp>

using namespace pragma;

LINK_ENTITY_TO_CLASS(point_constraint_ballsocket,PointConstraintBallSocket);

void SPointConstraintBallSocketComponent::SendData(NetPacket &packet,nwm::RecipientFilter &rp)
{
	packet->WriteString(m_kvSource);
	packet->WriteString(m_kvTarget);
	nwm::write_vector(packet,m_posTarget);
}

luabind::object SPointConstraintBallSocketComponent::InitializeLuaObject(lua_State *l) {return BaseEntityComponent::InitializeLuaObject<SPointConstraintBallSocketComponentHandleWrapper>(l);}

///////

void PointConstraintBallSocket::Initialize()
{
	SBaseEntity::Initialize();
	AddComponent<SPointConstraintBallSocketComponent>();
}
