#include "stdafx_server.h"
#include "pragma/entities/point/constraints/s_point_constraint_hinge.h"
#include "pragma/entities/s_entityfactories.h"
#include <pragma/physics/physobj.h>
#include <sharedutils/util_string.h>
#include <pragma/networking/nwm_util.h>
#include "pragma/lua/s_lentity_handles.hpp"

using namespace pragma;

LINK_ENTITY_TO_CLASS(point_constraint_hinge,PointConstraintHinge);

void SPointConstraintHingeComponent::SendData(NetPacket &packet,nwm::RecipientFilter &rp)
{
	packet->WriteString(m_kvSource);
	packet->WriteString(m_kvTarget);
	nwm::write_vector(packet,m_posTarget);

	packet->Write<float>(m_kvLimitLow);
	packet->Write<float>(m_kvLimitHigh);
	packet->Write<float>(m_kvLimitSoftness);
	packet->Write<float>(m_kvLimitBiasFactor);
	packet->Write<float>(m_kvLimitRelaxationFactor);
}

luabind::object SPointConstraintHingeComponent::InitializeLuaObject(lua_State *l) {return BaseEntityComponent::InitializeLuaObject<SPointConstraintHingeComponentHandleWrapper>(l);}

void PointConstraintHinge::Initialize()
{
	SBaseEntity::Initialize();
	AddComponent<SPointConstraintHingeComponent>();
}