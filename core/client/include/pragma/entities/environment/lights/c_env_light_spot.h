#ifndef __C_ENV_LIGHT_SPOT_H__
#define __C_ENV_LIGHT_SPOT_H__
#include "pragma/clientdefinitions.h"
#include "pragma/entities/c_baseentity.h"
#include "pragma/entities/environment/lights/c_env_light.h"
#include <pragma/entities/environment/lights/env_light_spot.h>
#include <pragma/util/mvpbase.h>

namespace pragma
{
	class DLLCLIENT CLightSpotComponent final
		: public BaseEnvLightSpotComponent,
		public CBaseNetComponent,
		public MVPBias<1>
	{
	public:
		CLightSpotComponent(BaseEntity &ent);
		virtual void Initialize() override;
		virtual luabind::object InitializeLuaObject(lua_State *l) override;
		virtual bool ShouldTransmitNetData() const override {return true;}
	protected:
		virtual void OnEntityComponentAdded(BaseEntityComponent &component) override;
		virtual void ReceiveData(NetPacket &packet) override;

		void UpdateViewMatrices();
		void UpdateProjectionMatrix();
		void UpdateTransformMatrix();
	};
};

class DLLCLIENT CEnvLightSpot
	: public CBaseEntity
{
public:
	virtual void Initialize() override;
};

#endif