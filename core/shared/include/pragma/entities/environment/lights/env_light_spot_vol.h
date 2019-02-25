#ifndef __ENV_LIGHT_SPOT_VOL_H__
#define __ENV_LIGHT_SPOT_VOL_H__

#include "pragma/entities/components/base_entity_component.hpp"
#include "pragma/entities/components/basetoggle.h"
#include <mathutil/color.h>
#include <mathutil/umath.h>
#include <string>

namespace pragma
{
	class DLLNETWORK BaseEnvLightSpotVolComponent
		: public BaseEntityComponent
	{
	public:
		using BaseEntityComponent::BaseEntityComponent;
		virtual void Initialize() override;
	protected:
		virtual void OnEntityComponentAdded(BaseEntityComponent &component) override;
		float m_coneLength = 100.f;
		float m_coneAngle = 45.f;
		Color m_coneColor;
	};
};

#endif