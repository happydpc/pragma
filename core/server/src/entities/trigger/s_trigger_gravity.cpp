#include "stdafx_server.h"
#include "pragma/entities/trigger/s_trigger_gravity.h"
#include "pragma/entities/s_entityfactories.h"
#include <pragma/physics/physobj.h>
#include <sharedutils/util_string.h>
#include <sharedutils/util.h>
#include "pragma/entities/player.h"
#include "pragma/lua/s_lentity_handles.hpp"
#include <pragma/entities/baseentity_events.hpp>
#include <pragma/util/util_handled.hpp>
#include <pragma/entities/components/basetriggergravity.hpp>

using namespace pragma;

LINK_ENTITY_TO_CLASS(trigger_gravity,TriggerGravity);

void STriggerGravityComponent::Initialize()
{
	BaseEntityComponent::Initialize();

	BindEvent(BaseEntity::EVENT_HANDLE_KEY_VALUE,[this](std::reference_wrapper<pragma::ComponentEvent> evData) -> util::EventReply {
		auto &kvData = static_cast<CEKeyValueData&>(evData.get());
		if(ustring::compare(kvData.key,"gravity_dir",false))
		{
			EulerAngles ang(kvData.value);
			m_kvGravityDir = ang.Forward();
		}
		else if(ustring::compare(kvData.key,"gravity_force",false))
			m_kvGravityForce = util::to_float(kvData.value);
		else if(ustring::compare(kvData.key,"use_force",false))
			m_kvUseForce = util::to_boolean(kvData.value);
		else
			return util::EventReply::Unhandled;
		return util::EventReply::Handled;
	});
}

void STriggerGravityComponent::OnResetGravity(BaseEntity *ent,GravitySettings &settings)
{
	BaseEntityTriggerGravityComponent::OnResetGravity(ent,settings);
	auto &entThis = GetEntity();
	NetPacket p {};
	nwm::write_entity(p,ent);
	p->Write<uint8_t>(umath::to_integral(pragma::Entity::TriggerGravity::NetFlags::None));
	p->Write<uint32_t>(entThis.GetSpawnFlags());
	p->Write<Vector3>((settings.dir != nullptr) ? *settings.dir : Vector3{});
	p->Write<float>((settings.force != nullptr) ? *settings.force : 0.f);
	p->Write<Vector3>((settings.dirMove != nullptr) ? *settings.dirMove : Vector3{});
	server->BroadcastTCP("ent_trigger_gravity_onstarttouch",p);
}

void STriggerGravityComponent::OnStartTouch(BaseEntity *ent,PhysObj *phys)
{
	BaseEntityTriggerGravityComponent::OnStartTouch(ent,phys);
	auto netFlags = pragma::Entity::TriggerGravity::NetFlags::StartTouch;
	if(m_kvUseForce == true)
		netFlags |= pragma::Entity::TriggerGravity::NetFlags::UseForce;
	auto &entThis = GetEntity();
	NetPacket p {};
	nwm::write_entity(p,ent);
	p->Write<uint8_t>(umath::to_integral(netFlags));
	p->Write<uint32_t>(entThis.GetSpawnFlags());
	p->Write<Vector3>(m_kvGravityDir);
	p->Write<float>(m_kvGravityForce);
	server->BroadcastTCP("ent_trigger_gravity_onstarttouch",p);
}

////////////

luabind::object STriggerGravityComponent::InitializeLuaObject(lua_State *l) {return BaseEntityComponent::InitializeLuaObject<STriggerGravityComponentHandleWrapper>(l);}

void TriggerGravity::Initialize()
{
	SBaseEntity::Initialize();
	AddComponent<STriggerGravityComponent>();
}