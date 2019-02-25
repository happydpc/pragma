#include "stdafx_shared.h"
#include "pragma/entities/environment/effects/env_fire.h"
#include "pragma/util/util_handled.hpp"
#include "pragma/entities/baseentity_events.hpp"
#include <algorithm>

using namespace pragma;

void BaseEnvFireComponent::Initialize()
{
	BaseEntityComponent::Initialize();
	GetEntity().AddComponent("toggle");

	BindEvent(BaseEntity::EVENT_HANDLE_KEY_VALUE,[this](std::reference_wrapper<pragma::ComponentEvent> evData) -> util::EventReply {
		auto &kvData = static_cast<CEKeyValueData&>(evData.get());
		if(ustring::compare(kvData.key,"fire_type",false))
			m_fireType = kvData.value;
		else
			return util::EventReply::Unhandled;
		return util::EventReply::Handled;
	});
}