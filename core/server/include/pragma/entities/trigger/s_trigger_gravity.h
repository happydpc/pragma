#ifndef __S_TRIGGER_GRAVITY_H__
#define __S_TRIGGER_GRAVITY_H__
#include "pragma/serverdefinitions.h"
#include "pragma/entities/s_baseentity.h"
#include "pragma/entities/trigger/s_trigger_touch.h"
#include <pragma/entities/components/basetriggergravity.hpp>

namespace pragma
{
	class DLLSERVER STriggerGravityComponent final
		: public BaseEntityTriggerGravityComponent
	{
	public:
		STriggerGravityComponent(BaseEntity &ent) : BaseEntityTriggerGravityComponent(ent) {}
		virtual void Initialize() override;
		virtual luabind::object InitializeLuaObject(lua_State *l) override;
	protected:
		virtual void OnStartTouch(BaseEntity *ent,PhysObj *phys) override;
		virtual void OnResetGravity(BaseEntity *ent,GravitySettings &settings) override;
	};
};

class DLLSERVER TriggerGravity
	: public SBaseEntity
{
public:
	virtual void Initialize() override;
};

#endif